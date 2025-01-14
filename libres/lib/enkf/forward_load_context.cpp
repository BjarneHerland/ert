/*
   Copyright (C) 2016  Equinor ASA, Norway.

   The file 'forward_load_context.c.h' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

#include <cstdlib>
#include <ert/util/stringlist.h>
#include <ert/util/type_macros.h>

#include <ert/enkf/enkf_defaults.hpp>
#include <ert/enkf/forward_load_context.hpp>
#include <ert/enkf/run_arg.hpp>
#include <ert/res_util/memory.hpp>
#include <fmt/format.h>

#define FORWARD_LOAD_CONTEXT_TYPE_ID 644239127

static auto logger = ert::get_logger("enkf.forward_load_context");

struct forward_load_context_struct {
    UTIL_TYPE_ID_DECLARATION;
    // Everyuthing can be NULL here ... - when created from gen_data.

    ecl_sum_type *ecl_sum;
    ecl_file_type *restart_file;
    const run_arg_type *run_arg;
    /** Can be NULL */
    const ecl_config_type *ecl_config;

    int step2;
    /** Messages is managed by external scope - can be NULL */
    stringlist_type *messages;

    /* The variables below are updated during the load process. */
    int load_step;
    fw_load_status load_result;
    bool ecl_active;
};

UTIL_IS_INSTANCE_FUNCTION(forward_load_context, FORWARD_LOAD_CONTEXT_TYPE_ID);

static void
forward_load_context_load_ecl_sum(forward_load_context_type *load_context) {
    ecl_sum_type *summary = NULL;

    if (load_context->ecl_active) {
        const run_arg_type *run_arg =
            forward_load_context_get_run_arg(load_context);
        const char *run_path = run_arg_get_runpath(run_arg);
        const char *eclbase = run_arg_get_job_name(load_context->run_arg);

        const bool fmt_file =
            ecl_config_get_formatted(load_context->ecl_config);
        char *header_file = ecl_util_alloc_exfilename(
            run_path, eclbase, ECL_SUMMARY_HEADER_FILE, fmt_file, -1);
        char *unified_file = ecl_util_alloc_exfilename(
            run_path, eclbase, ECL_UNIFIED_SUMMARY_FILE, fmt_file, -1);
        stringlist_type *data_files = stringlist_alloc_new();

        if ((unified_file != NULL) && (header_file != NULL)) {
            stringlist_append_copy(data_files, unified_file);

            bool include_restart = false;

            /*
             * Setting this flag causes summary-data to be loaded by
             * ecl::unsmry_loader which is "horribly slow" according
             * to comments in the code. The motivation for introducing
             * this mode was at some point to use less memory, but
             * computers nowadays should not have a problem with that.
             *
             * For comments, reasoning and discussions, please refer to
             * https://github.com/equinor/ert/issues/2873
             *   and
             * https://github.com/equinor/ert/issues/2972
             */
            bool lazy_load = false;
            if (std::getenv("ERT_LAZY_LOAD_SUMMARYDATA"))
                lazy_load = true;

            {
                ert::utils::scoped_memory_logger memlogger(
                    logger, fmt::format("lazy={}", lazy_load));

                int file_options = 0;
                summary = ecl_sum_fread_alloc(
                    header_file, data_files, SUMMARY_KEY_JOIN_STRING,
                    include_restart, lazy_load, file_options);
            }

            {
                time_t end_time =
                    ecl_config_get_end_date(load_context->ecl_config);
                if (end_time > 0) {
                    if (ecl_sum_get_end_time(summary) < end_time) {
                        /* The summary vector was shorter than expected; we interpret this as
               a simulation failure and discard the current summary instance. */
                        logger->error("The summary vector was shorter (end: "
                                      "{}) than expected (end: {})",
                                      ecl_sum_get_end_time(summary), end_time);
                    }
                    ecl_sum_free(summary);
                    summary = NULL;
                }
            }
        } else
            logger->error("Could not find SUMMARY file at: {}/{} or using non "
                          "unified SUMMARY file",
                          run_path, eclbase);

        stringlist_free(data_files);
        free(header_file);
        free(unified_file);
    }

    if (summary)
        load_context->ecl_sum = summary;
    else
        forward_load_context_update_result(load_context, LOAD_FAILURE);
}

forward_load_context_type *
forward_load_context_alloc(const run_arg_type *run_arg, bool load_summary,
                           const ecl_config_type *ecl_config) {
    forward_load_context_type *load_context =
        (forward_load_context_type *)util_malloc(sizeof *load_context);
    UTIL_TYPE_ID_INIT(load_context, FORWARD_LOAD_CONTEXT_TYPE_ID);

    load_context->ecl_active = false;
    load_context->ecl_sum = NULL;
    load_context->restart_file = NULL;
    load_context->run_arg = run_arg;
    load_context->load_step =
        -1; // Invalid - must call forward_load_context_select_step()
    load_context->load_result = LOAD_SUCCESSFUL;
    load_context->ecl_config = ecl_config;
    if (ecl_config)
        load_context->ecl_active = ecl_config_active(ecl_config);

    if (load_summary)
        forward_load_context_load_ecl_sum(load_context);

    return load_context;
}

fw_load_status
forward_load_context_get_result(const forward_load_context_type *load_context) {
    return load_context->load_result;
}

void forward_load_context_update_result(forward_load_context_type *load_context,
                                        fw_load_status flags) {
    load_context->load_result = flags;
}

void forward_load_context_free(forward_load_context_type *load_context) {
    if (load_context->restart_file)
        ecl_file_close(load_context->restart_file);

    if (load_context->ecl_sum)
        ecl_sum_free(load_context->ecl_sum);

    free(load_context);
}

bool forward_load_context_load_restart_file(
    forward_load_context_type *load_context, int report_step) {
    if (load_context->ecl_config) {
        const bool unified =
            ecl_config_get_unified_restart(load_context->ecl_config);
        if (unified)
            util_abort("%s: sorry - unified restart files are not supported \n",
                       __func__);

        forward_load_context_select_step(load_context, report_step);
        {
            const bool fmt_file =
                ecl_config_get_formatted(load_context->ecl_config);
            char *filename = ecl_util_alloc_exfilename(
                run_arg_get_runpath(load_context->run_arg),
                run_arg_get_job_name(load_context->run_arg), ECL_RESTART_FILE,
                fmt_file, load_context->load_step);

            if (load_context->restart_file)
                ecl_file_close(load_context->restart_file);
            load_context->restart_file = NULL;

            if (filename) {
                load_context->restart_file = ecl_file_open(filename, 0);
                free(filename);
            }

            if (load_context->restart_file)
                return true;
            else
                return false;
        }
    } else {
        util_abort("%s: internal error - tried to load restart with "
                   "load_context with ecl_config==NULL \n",
                   __func__);
        return false;
    }
}

const ecl_sum_type *forward_load_context_get_ecl_sum(
    const forward_load_context_type *load_context) {
    return load_context->ecl_sum;
}

const run_arg_type *forward_load_context_get_run_arg(
    const forward_load_context_type *load_context) {
    return load_context->run_arg;
}

const char *forward_load_context_get_run_path(
    const forward_load_context_type *load_context) {
    return run_arg_get_runpath(load_context->run_arg);
}

enkf_fs_type *
forward_load_context_get_sim_fs(const forward_load_context_type *load_context) {
    return run_arg_get_sim_fs(load_context->run_arg);
}

void forward_load_context_select_step(forward_load_context_type *load_context,
                                      int report_step) {
    load_context->load_step = report_step;
}

int forward_load_context_get_load_step(
    const forward_load_context_type *load_context) {
    if (load_context->load_step < 0)
        util_abort("%s: this looks like an internal error - missing call to "
                   "forward_load_context_select_step() \n",
                   __func__);

    return load_context->load_step;
}

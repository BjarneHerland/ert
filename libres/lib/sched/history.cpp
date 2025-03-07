/*
   Copyright (C) 2011  Equinor ASA, Norway.
   The file 'history.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <stdio.h>
#include <string.h>

#include <ert/util/bool_vector.hpp>
#include <ert/util/hash.hpp>
#include <ert/util/type_macros.hpp>

#include <ert/ecl/ecl_sum.h>

#include <ert/sched/history.hpp>

#define HISTORY_TYPE_ID 66143109

struct history_struct {
    UTIL_TYPE_ID_DECLARATION;
    /** ecl_sum instance used when the data are taken from a summary instance.
     * Observe that this is NOT owned by history instance.*/
    const ecl_sum_type *refcase;
    history_source_type source;
};

history_source_type history_get_source_type(const char *string_source) {
    history_source_type source_type = HISTORY_SOURCE_INVALID;

    if (strcmp(string_source, "REFCASE_SIMULATED") == 0)
        source_type = REFCASE_SIMULATED;
    else if (strcmp(string_source, "REFCASE_HISTORY") == 0)
        source_type = REFCASE_HISTORY;
    else
        util_abort("%s: Sorry source:%s not recognized\n", __func__,
                   string_source);

    return source_type;
}

const char *history_get_source_string(history_source_type history_source) {
    switch (history_source) {
    case (REFCASE_SIMULATED):
        return "REFCASE_SIMULATED";
        break;
    case (REFCASE_HISTORY):
        return "REFCASE_HISTORY";
        break;
    default:
        util_abort("%s: Internal inconsistency in refcase \n", __func__);
        return NULL;
    }
}

UTIL_IS_INSTANCE_FUNCTION(history, HISTORY_TYPE_ID)

static history_type *history_alloc_empty() {
    history_type *history = (history_type *)util_malloc(sizeof *history);
    UTIL_TYPE_ID_INIT(history, HISTORY_TYPE_ID);
    history->refcase = NULL;
    return history;
}

void history_free(history_type *history) { free(history); }

history_type *history_alloc_from_refcase(const ecl_sum_type *refcase,
                                         bool use_h_keywords) {
    history_type *history = history_alloc_empty();

    history->refcase =
        refcase; /* This function does not really do anthing - it just sets the ecl_sum field of the history instance. */
    if (use_h_keywords)
        history->source = REFCASE_HISTORY;
    else
        history->source = REFCASE_SIMULATED;

    return history;
}

history_source_type history_get_source(const history_type *history) {
    return history->source;
}

int history_get_last_restart(const history_type *history) {
    return ecl_sum_get_last_report_step(history->refcase);
}

bool history_init_ts(const history_type *history, const char *summary_key,
                     double_vector_type *value, bool_vector_type *valid) {
    bool initOK = false;

    double_vector_reset(value);
    bool_vector_reset(valid);
    bool_vector_set_default(valid, false);

    char *local_key;
    if (history->source == REFCASE_HISTORY) {
        /* Must create a new key with 'H' for historical values. */
        const ecl_smspec_type *smspec = ecl_sum_get_smspec(history->refcase);
        const char *join_string = ecl_smspec_get_join_string(smspec);
        ecl_smspec_var_type var_type =
            ecl_smspec_identify_var_type(summary_key);

        if ((var_type == ECL_SMSPEC_WELL_VAR) ||
            (var_type == ECL_SMSPEC_GROUP_VAR))
            local_key = util_alloc_sprintf(
                "%sH%s%s", ecl_sum_get_keyword(history->refcase, summary_key),
                join_string, ecl_sum_get_wgname(history->refcase, summary_key));
        else if (var_type == ECL_SMSPEC_FIELD_VAR)
            local_key = util_alloc_sprintf(
                "%sH", ecl_sum_get_keyword(history->refcase, summary_key));
        else
            local_key =
                NULL; // If we try to get historical values of e.g. Region quantities it will fail.
    } else
        local_key = (char *)summary_key;

    if (local_key) {
        if (ecl_sum_has_general_var(history->refcase, local_key)) {
            for (int tstep = 0; tstep <= history_get_last_restart(history);
                 tstep++) {
                if (ecl_sum_has_report_step(history->refcase, tstep)) {
                    int time_index =
                        ecl_sum_iget_report_end(history->refcase, tstep);
                    double_vector_iset(value, tstep,
                                       ecl_sum_get_general_var(history->refcase,
                                                               time_index,
                                                               local_key));
                    bool_vector_iset(valid, tstep, true);
                } else
                    bool_vector_iset(valid, tstep,
                                     false); /* Did not have this report step */
            }
            initOK = true;
        }

        if (history->source == REFCASE_HISTORY)
            free(local_key);
    }
    return initOK;
}

time_t history_get_start_time(const history_type *history) {
    return ecl_sum_get_start_time(history->refcase);
}

/* Uncertain about the first node - offset problems +++ ??
   Changed to use node_end_time() at svn ~ 2850

   Changed to sched_history at svn ~2940
*/
time_t history_get_time_t_from_restart_nr(const history_type *history,
                                          int restart_nr) {
    if (restart_nr == 0)
        return ecl_sum_get_start_time(history->refcase);
    else
        return ecl_sum_get_report_time(history->refcase, restart_nr);
}

int history_get_restart_nr_from_time_t(const history_type *history,
                                       time_t time) {
    if (time == history_get_start_time(history))
        return 0;
    else {
        int report_step =
            ecl_sum_get_report_step_from_time(history->refcase, time);
        if (report_step >= 1)
            return report_step;
        else {
            int mday, year, month;
            util_set_date_values_utc(time, &mday, &month, &year);
            util_abort("%s: Date: %02d/%02d/%04d  does not cooincide with any "
                       "report time. Aborting.\n",
                       __func__, mday, month, year);
            return -1;
        }
    }
}

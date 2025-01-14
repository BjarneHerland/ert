/*
   Copyright (C) 2013  Equinor ASA, Norway.

   The file 'torque_driver.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <filesystem>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ert/res_util/file_utils.hpp>
#include <ert/util/type_macros.hpp>
#include <ert/util/util.hpp>

#include <ert/job_queue/torque_driver.hpp>

namespace fs = std::filesystem;

#define TORQUE_DRIVER_TYPE_ID 34873653
#define TORQUE_JOB_TYPE_ID 12312312

struct torque_driver_struct {
    UTIL_TYPE_ID_DECLARATION;
    char *queue_name;
    char *qsub_cmd;
    char *qstat_cmd;
    char *qdel_cmd;
    char *num_cpus_per_node_char;
    char *job_prefix;
    char *num_nodes_char;
    bool keep_qsub_output;
    int num_cpus_per_node;
    int num_nodes;
    char *cluster_label;
    int submit_sleep;
    FILE *debug_stream;
};

struct torque_job_struct {
    UTIL_TYPE_ID_DECLARATION;
    long int torque_jobnr;
    char *torque_jobnr_char;
};

UTIL_SAFE_CAST_FUNCTION(torque_driver, TORQUE_DRIVER_TYPE_ID);

static UTIL_SAFE_CAST_FUNCTION_CONST(torque_driver, TORQUE_DRIVER_TYPE_ID);
static UTIL_SAFE_CAST_FUNCTION(torque_job, TORQUE_JOB_TYPE_ID);

void *torque_driver_alloc() {
    torque_driver_type *torque_driver =
        (torque_driver_type *)util_malloc(sizeof *torque_driver);
    UTIL_TYPE_ID_INIT(torque_driver, TORQUE_DRIVER_TYPE_ID);

    torque_driver->queue_name = NULL;
    torque_driver->qsub_cmd = NULL;
    torque_driver->qstat_cmd = NULL;
    torque_driver->qdel_cmd = NULL;
    torque_driver->num_cpus_per_node_char = NULL;
    torque_driver->num_nodes_char = NULL;
    torque_driver->keep_qsub_output = false;
    torque_driver->num_cpus_per_node = 1;
    torque_driver->num_nodes = 1;
    torque_driver->cluster_label = NULL;
    torque_driver->job_prefix = NULL;
    torque_driver->debug_stream = NULL;

    torque_driver_set_option(torque_driver, TORQUE_QSUB_CMD,
                             TORQUE_DEFAULT_QSUB_CMD);
    torque_driver_set_option(torque_driver, TORQUE_QSTAT_CMD,
                             TORQUE_DEFAULT_QSTAT_CMD);
    torque_driver_set_option(torque_driver, TORQUE_QDEL_CMD,
                             TORQUE_DEFAULT_QDEL_CMD);
    torque_driver_set_option(torque_driver, TORQUE_NUM_CPUS_PER_NODE, "1");
    torque_driver_set_option(torque_driver, TORQUE_NUM_NODES, "1");
    torque_driver_set_option(torque_driver, TORQUE_SUBMIT_SLEEP,
                             TORQUE_DEFAULT_SUBMIT_SLEEP);

    return torque_driver;
}

static void torque_driver_set_debug_output(torque_driver_type *driver,
                                           const char *debug_file) {
    if (driver->debug_stream)
        fclose(driver->debug_stream);

    if (debug_file)
        driver->debug_stream = mkdir_fopen(fs::path(debug_file), "w");
    else
        driver->debug_stream = NULL;
}

static void torque_driver_set_qsub_cmd(torque_driver_type *driver,
                                       const char *qsub_cmd) {
    driver->qsub_cmd = util_realloc_string_copy(driver->qsub_cmd, qsub_cmd);
}

static void torque_driver_set_qstat_cmd(torque_driver_type *driver,
                                        const char *qstat_cmd) {
    driver->qstat_cmd = util_realloc_string_copy(driver->qstat_cmd, qstat_cmd);
}

static void torque_driver_set_qdel_cmd(torque_driver_type *driver,
                                       const char *qdel_cmd) {
    driver->qdel_cmd = util_realloc_string_copy(driver->qdel_cmd, qdel_cmd);
}

static void torque_driver_set_queue_name(torque_driver_type *driver,
                                         const char *queue_name) {
    driver->queue_name =
        util_realloc_string_copy(driver->queue_name, queue_name);
}

static bool torque_driver_set_submit_sleep(torque_driver_type *driver,
                                           const char *submit_sleep) {
    double seconds_sleep;
    if (util_sscanf_double(submit_sleep, &seconds_sleep)) {
        driver->submit_sleep = (int)(seconds_sleep * 1000000);
        return true;
    } else
        return false;
}

static bool torque_driver_set_num_nodes(torque_driver_type *driver,
                                        const char *num_nodes_char) {
    int num_nodes = 0;
    if (util_sscanf_int(num_nodes_char, &num_nodes)) {
        driver->num_nodes = num_nodes;
        driver->num_nodes_char =
            util_realloc_string_copy(driver->num_nodes_char, num_nodes_char);
        return true;
    } else {
        return false;
    }
}

static bool
torque_driver_set_keep_qsub_output(torque_driver_type *driver,
                                   const char *keep_output_bool_as_char) {
    bool keep_output_parsed;

    if (util_sscanf_bool(keep_output_bool_as_char, &keep_output_parsed)) {
        driver->keep_qsub_output = keep_output_parsed;
        return true;
    } else {
        return false;
    }
}

static void torque_driver_set_job_prefix(torque_driver_type *driver,
                                         const char *job_prefix) {
    driver->job_prefix =
        util_realloc_string_copy(driver->job_prefix, job_prefix);
}

static void torque_driver_set_cluster_label(torque_driver_type *driver,
                                            const char *cluster_label) {
    driver->cluster_label =
        util_realloc_string_copy(driver->cluster_label, cluster_label);
}

static bool
torque_driver_set_num_cpus_per_node(torque_driver_type *driver,
                                    const char *num_cpus_per_node_char) {
    int num_cpus_per_node = 0;
    if (util_sscanf_int(num_cpus_per_node_char, &num_cpus_per_node)) {
        driver->num_cpus_per_node = num_cpus_per_node;
        driver->num_cpus_per_node_char = util_realloc_string_copy(
            driver->num_cpus_per_node_char, num_cpus_per_node_char);
        return true;
    } else {
        return false;
    }
}

bool torque_driver_set_option(void *__driver, const char *option_key,
                              const void *value_) {
    const char *value = (const char *)value_;
    torque_driver_type *driver = torque_driver_safe_cast(__driver);
    bool option_set = true;
    {
        if (strcmp(TORQUE_QSUB_CMD, option_key) == 0)
            torque_driver_set_qsub_cmd(driver, value);
        else if (strcmp(TORQUE_QSTAT_CMD, option_key) == 0)
            torque_driver_set_qstat_cmd(driver, value);
        else if (strcmp(TORQUE_QDEL_CMD, option_key) == 0)
            torque_driver_set_qdel_cmd(driver, value);
        else if (strcmp(TORQUE_QUEUE, option_key) == 0)
            torque_driver_set_queue_name(driver, value);
        else if (strcmp(TORQUE_NUM_CPUS_PER_NODE, option_key) == 0)
            option_set = torque_driver_set_num_cpus_per_node(driver, value);
        else if (strcmp(TORQUE_NUM_NODES, option_key) == 0)
            option_set = torque_driver_set_num_nodes(driver, value);
        else if (strcmp(TORQUE_KEEP_QSUB_OUTPUT, option_key) == 0)
            option_set = torque_driver_set_keep_qsub_output(driver, value);
        else if (strcmp(TORQUE_CLUSTER_LABEL, option_key) == 0)
            torque_driver_set_cluster_label(driver, value);
        else if (strcmp(TORQUE_JOB_PREFIX_KEY, option_key) == 0)
            torque_driver_set_job_prefix(driver, value);
        else if (strcmp(TORQUE_DEBUG_OUTPUT, option_key) == 0)
            torque_driver_set_debug_output(driver, value);
        else if (strcmp(TORQUE_SUBMIT_SLEEP, option_key) == 0)
            option_set = torque_driver_set_submit_sleep(driver, value);
        else
            option_set = false;
    }
    return option_set;
}

const void *torque_driver_get_option(const void *__driver,
                                     const char *option_key) {
    const torque_driver_type *driver = torque_driver_safe_cast_const(__driver);
    {
        if (strcmp(TORQUE_QSUB_CMD, option_key) == 0)
            return driver->qsub_cmd;
        else if (strcmp(TORQUE_QSTAT_CMD, option_key) == 0)
            return driver->qstat_cmd;
        else if (strcmp(TORQUE_QDEL_CMD, option_key) == 0)
            return driver->qdel_cmd;
        else if (strcmp(TORQUE_QUEUE, option_key) == 0)
            return driver->queue_name;
        else if (strcmp(TORQUE_NUM_CPUS_PER_NODE, option_key) == 0)
            return driver->num_cpus_per_node_char;
        else if (strcmp(TORQUE_NUM_NODES, option_key) == 0)
            return driver->num_nodes_char;
        else if (strcmp(TORQUE_KEEP_QSUB_OUTPUT, option_key) == 0)
            return driver->keep_qsub_output ? "1" : "0";
        else if (strcmp(TORQUE_CLUSTER_LABEL, option_key) == 0)
            return driver->cluster_label;
        else if (strcmp(TORQUE_JOB_PREFIX_KEY, option_key) == 0)
            return driver->job_prefix;
        else {
            util_abort("%s: option_id:%s not recognized for TORQUE driver \n",
                       __func__, option_key);
            return NULL;
        }
    }
}

void torque_driver_init_option_list(stringlist_type *option_list) {
    stringlist_append_copy(option_list, TORQUE_QSUB_CMD);
    stringlist_append_copy(option_list, TORQUE_QSTAT_CMD);
    stringlist_append_copy(option_list, TORQUE_QDEL_CMD);
    stringlist_append_copy(option_list, TORQUE_QUEUE);
    stringlist_append_copy(option_list, TORQUE_NUM_CPUS_PER_NODE);
    stringlist_append_copy(option_list, TORQUE_NUM_NODES);
    stringlist_append_copy(option_list, TORQUE_KEEP_QSUB_OUTPUT);
    stringlist_append_copy(option_list, TORQUE_CLUSTER_LABEL);
    stringlist_append_copy(option_list, TORQUE_JOB_PREFIX_KEY);
}

torque_job_type *torque_job_alloc() {
    torque_job_type *job;
    job = (torque_job_type *)util_malloc(sizeof *job);
    job->torque_jobnr_char = NULL;
    job->torque_jobnr = 0;
    UTIL_TYPE_ID_INIT(job, TORQUE_JOB_TYPE_ID);

    return job;
}

stringlist_type *torque_driver_alloc_cmd(torque_driver_type *driver,
                                         const char *job_name,
                                         const char *submit_script) {

    stringlist_type *argv = stringlist_alloc_new();

    if (driver->keep_qsub_output) {
        stringlist_append_copy(argv, "-k");
        stringlist_append_copy(argv, "oe");
    }

    {
        char *resource_string;
        if (driver->cluster_label)
            resource_string = util_alloc_sprintf(
                "nodes=%d:%s:ppn=%d", driver->num_nodes, driver->cluster_label,
                driver->num_cpus_per_node);
        else
            resource_string =
                util_alloc_sprintf("nodes=%d:ppn=%d", driver->num_nodes,
                                   driver->num_cpus_per_node);

        stringlist_append_copy(argv, "-l");
        stringlist_append_copy(argv, resource_string);
        free(resource_string);
    }

    if (driver->queue_name != NULL) {
        stringlist_append_copy(argv, "-q");
        stringlist_append_copy(argv, driver->queue_name);
    }

    if (job_name != NULL) {
        stringlist_append_copy(argv, "-N");
        stringlist_append_copy(argv, job_name);
    }

    stringlist_append_copy(argv, submit_script);

    return argv;
}

static void torque_debug(const torque_driver_type *driver, const char *fmt,
                         ...) {
    if (driver->debug_stream) {
        {
            va_list ap;
            va_start(ap, fmt);
            vfprintf(driver->debug_stream, fmt, ap);
            va_end(ap);
        }
        fprintf(driver->debug_stream, "\n");
        fsync(fileno(driver->debug_stream));
    }
}

static int torque_job_parse_qsub_stdout(const torque_driver_type *driver,
                                        const char *stdout_file) {
    int jobid;
    {
        FILE *stream = util_fopen(stdout_file, "r");
        char *jobid_string = util_fscanf_alloc_upto(stream, ".", false);

        torque_debug(driver, "Torque job ID string: '%s'", jobid_string);

        if (jobid_string == NULL || !util_sscanf_int(jobid_string, &jobid)) {

            char *file_content =
                util_fread_alloc_file_content(stdout_file, NULL);
            fprintf(stderr, "Failed to get torque job id from file: %s \n",
                    stdout_file);
            fprintf(stderr, "qsub command                      : %s \n",
                    driver->qsub_cmd);
            fprintf(stderr, "File content: [%s]\n", file_content);
            free(file_content);
            util_exit("%s: \n", __func__);
        }
        free(jobid_string);
        fclose(stream);
    }
    return jobid;
}

void torque_job_create_submit_script(const char *script_filename,
                                     const char *submit_cmd, int argc,
                                     const char **job_argv) {
    if (submit_cmd == NULL) {
        util_abort("%s: cannot create submit script, because there is no "
                   "executing commmand specified.",
                   __func__);
    }

    FILE *script_file = util_fopen(script_filename, "w");
    fprintf(script_file, "#!/bin/sh\n");

    fprintf(script_file, "%s", submit_cmd);
    for (int i = 0; i < argc; i++) {
        fprintf(script_file, " %s", job_argv[i]);
    }

    fclose(script_file);
}

static void torque_debug_spawn_status_info(torque_driver_type *driver,
                                           int status) {
    if (WIFEXITED((status))) {
        torque_debug(driver, "Torque spawn exited with status=%d",
                     WEXITSTATUS((status)));
    } else if (WIFSIGNALED((status))) {
        torque_debug(driver, "Torque spawn killed by signal %d",
                     WTERMSIG((status)));
    } else if (WIFSTOPPED((status))) {
        torque_debug(driver, "Torque spawn stopped by signal %d",
                     WSTOPSIG((status)));
    } else if (WIFCONTINUED((status))) {
        torque_debug(driver, "Torque spawn continued");
    } else {
        torque_debug(driver, "Torque spawn failed with unknown status code: %d",
                     (status));
    }
}

static int torque_driver_submit_shell_job(torque_driver_type *driver,
                                          const char *run_path,
                                          const char *job_name,
                                          const char *submit_cmd, int num_cpu,
                                          int job_argc, const char **job_argv) {

    usleep(driver->submit_sleep);
    {
        int job_id;
        char *tmp_std_file =
            (char *)util_alloc_tmp_file("/tmp", "enkf-submit-std", true);
        char *tmp_err_file =
            (char *)util_alloc_tmp_file("/tmp", "enkf-submit-err", true);
        char *script_filename =
            (char *)util_alloc_filename(run_path, "qsub_script", "sh");

        torque_debug(driver, "Setting up submit stdout target '%s' for '%s'",
                     tmp_std_file, script_filename);
        torque_debug(driver, "Setting up submit stderr target '%s' for '%s'",
                     tmp_err_file, script_filename);
        torque_job_create_submit_script(script_filename, submit_cmd, job_argc,
                                        job_argv);
        {
            int p_units_from_driver =
                driver->num_cpus_per_node * driver->num_nodes;
            if (num_cpu > p_units_from_driver) {
                util_abort("%s: Error in config, job's config requires %d "
                           "processing units, but config says %s: %d, and %s: "
                           "%d, which multiplied becomes: %d \n",
                           __func__, num_cpu, TORQUE_NUM_CPUS_PER_NODE,
                           driver->num_cpus_per_node, TORQUE_NUM_NODES,
                           driver->num_nodes, p_units_from_driver);
            }
            {
                stringlist_type *remote_argv =
                    torque_driver_alloc_cmd(driver, job_name, script_filename);
                torque_debug(driver, "Submit arguments: %s",
                             stringlist_alloc_joined_string(remote_argv, " "));
                char **argv = stringlist_alloc_char_ref(remote_argv);
                int status = util_spawn_blocking(
                    driver->qsub_cmd, stringlist_get_size(remote_argv),
                    (const char **)argv, tmp_std_file, tmp_err_file);
                if (status != 0) {
                    torque_debug_spawn_status_info(driver, status);
                }
                free(argv);
                stringlist_free(remote_argv);
            }
        }

        job_id = torque_job_parse_qsub_stdout(driver, tmp_std_file);

        util_unlink_existing(tmp_std_file);
        util_unlink_existing(tmp_err_file);
        free(tmp_std_file);
        free(tmp_err_file);

        return job_id;
    }
}

void torque_job_free(torque_job_type *job) {

    free(job->torque_jobnr_char);
    free(job);
}

void torque_driver_free_job(void *__job) {

    torque_job_type *job = torque_job_safe_cast(__job);
    torque_job_free(job);
}

void *torque_driver_submit_job(void *__driver, const char *submit_cmd,
                               int num_cpu, const char *run_path,
                               const char *job_name, int argc,
                               const char **argv) {
    torque_driver_type *driver = torque_driver_safe_cast(__driver);
    torque_job_type *job = torque_job_alloc();

    torque_debug(driver, "Submitting job in:%s", run_path);
    {
        char *local_job_name = NULL;
        if (driver->job_prefix)
            local_job_name =
                util_alloc_sprintf("%s%s", driver->job_prefix, job_name);
        else
            local_job_name = util_alloc_string_copy(job_name);

        job->torque_jobnr = torque_driver_submit_shell_job(
            driver, run_path, local_job_name, submit_cmd, num_cpu, argc, argv);
        job->torque_jobnr_char = util_alloc_sprintf("%ld", job->torque_jobnr);

        torque_debug(driver, "Job:%s Id:%d", run_path, job->torque_jobnr);
        free(local_job_name);
    }

    if (job->torque_jobnr > 0)
        return job;
    else {
        // The submit failed - the queue system shall handle
        // NULL return values.
        torque_job_free(job);
        return NULL;
    }
}

/**
   Will return NULL if "something" fails; that again will be
   translated to JOB_QUEUE_STATUS_FAILURE - which the queue layer will
   just interpret as "No change in status". Possible failures are:

    1. The file capturing stdout is not created.
    2. Can not extract the correct status string from the stdout file.

*/
static job_status_type
torque_driver_get_qstat_status(torque_driver_type *driver,
                               const char *jobnr_char) {
    char *tmp_file = (char *)util_alloc_tmp_file("/tmp", "enkf-qstat", true);
    job_status_type status = JOB_QUEUE_STATUS_FAILURE;

    {
        const char **argv = (const char **)util_calloc(1, sizeof *argv);
        argv[0] = jobnr_char;

        util_spawn_blocking(driver->qstat_cmd, 1, (const char **)argv, tmp_file,
                            NULL);
        free(argv);
    }

    if (fs::exists(tmp_file)) {
        status = torque_driver_parse_status(tmp_file, jobnr_char);
        unlink(tmp_file);
    } else
        fprintf(stderr, "No such file: %s - reading qstat status failed \n",
                tmp_file);

    free(tmp_file);

    return status;
}

job_status_type torque_driver_parse_status(const char *qstat_file,
                                           const char *jobnr_char) {
    job_status_type status = JOB_QUEUE_STATUS_FAILURE;

    if (fs::exists(qstat_file)) {
        char *line = NULL;
        {
            FILE *stream = util_fopen(qstat_file, "r");
            bool at_eof = false;
            util_fskip_lines(stream, 2);
            line = util_fscanf_alloc_line(stream, &at_eof);
            fclose(stream);
        }

        if (line) {
            char job_id_full_string[32];
            char string_status[2];

            if (sscanf(line, "%s %*s %*s %*s %s %*s", job_id_full_string,
                       string_status) == 2) {
                const char *dotPtr = strchr(job_id_full_string, '.');
                int dotPosition = dotPtr - job_id_full_string;

                {
                    char *job_id_as_char_ptr = util_alloc_substring_copy(
                        job_id_full_string, 0, dotPosition);
                    if (util_string_equal(job_id_as_char_ptr, jobnr_char)) {

                        switch (string_status[0]) {
                        case 'R':
                            status = JOB_QUEUE_RUNNING;
                            break;

                        case 'E':
                            status = JOB_QUEUE_DONE;
                            break;

                        case 'C':
                            status = JOB_QUEUE_DONE;
                            break;

                        case 'H':
                            status = JOB_QUEUE_PENDING;
                            break;
                        case 'Q':
                            status = JOB_QUEUE_PENDING;
                            break;

                        default:
                            break;
                        }

                        free(job_id_as_char_ptr);
                    }
                }
            }
            free(line);
        }
    }
    if (status == JOB_QUEUE_STATUS_FAILURE)
        fprintf(
            stderr,
            "** Warning: failed to get job status for job:%s from file:%s\n",
            jobnr_char, qstat_file);

    return status;
}

job_status_type torque_driver_get_job_status(void *__driver, void *__job) {
    torque_driver_type *driver = torque_driver_safe_cast(__driver);
    torque_job_type *job = torque_job_safe_cast(__job);
    return torque_driver_get_qstat_status(driver, job->torque_jobnr_char);
}

void torque_driver_kill_job(void *__driver, void *__job) {

    torque_driver_type *driver = torque_driver_safe_cast(__driver);
    torque_job_type *job = torque_job_safe_cast(__job);
    util_spawn_blocking(driver->qdel_cmd, 1,
                        (const char **)&job->torque_jobnr_char, NULL, NULL);
}

void torque_driver_free(torque_driver_type *driver) {
    torque_driver_set_debug_output(driver, NULL);
    free(driver->queue_name);
    free(driver->qdel_cmd);
    free(driver->qstat_cmd);
    free(driver->qsub_cmd);
    free(driver->num_cpus_per_node_char);
    free(driver->num_nodes_char);
    if (driver->job_prefix)
        free(driver->job_prefix);

    free(driver);
}

void torque_driver_free__(void *__driver) {
    torque_driver_type *driver = torque_driver_safe_cast(__driver);
    torque_driver_free(driver);
}

int torque_driver_get_submit_sleep(const torque_driver_type *driver) {
    return driver->submit_sleep;
}

FILE *torque_driver_get_debug_stream(const torque_driver_type *driver) {
    return driver->debug_stream;
}

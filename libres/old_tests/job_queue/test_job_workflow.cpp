/*
   Copyright (C) 2012  Equinor ASA, Norway.

   The file 'job_workflow_test.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>
#include <ert/util/util.hpp>

#include <ert/config/config_parser.hpp>

#include <ert/job_queue/workflow.hpp>
#include <ert/job_queue/workflow_job.hpp>
#include <ert/job_queue/workflow_joblist.hpp>

void create_workflow(const char *workflow_file, const char *tmp_file,
                     int value) {
    FILE *stream = util_fopen(workflow_file, "w");
    fprintf(stream, "CREATE_FILE   %s   %d\n", tmp_file, value);
    fprintf(stream, "READ_FILE     %s\n", tmp_file);
    fclose(stream);

    printf("Have created:%s \n", workflow_file);
}

void create_error_workflow(const char *workflow_file, const char *tmp_file,
                           int value) {
    FILE *stream = util_fopen(workflow_file, "w");
    fprintf(stream, "CREATE_FILE   %s   %d\n", tmp_file, value);
    fprintf(stream, "XREAD_FILE     %s\n", tmp_file);
    fclose(stream);

    printf("Have created:%s \n", workflow_file);
}

static void create_exjob(const char *workflow, const char *bin_path) {
    FILE *stream = util_fopen(workflow, "w");
    fprintf(stream, "EXECUTABLE  \"%s/create_file\"\n", bin_path);
    fprintf(stream, "ARG_TYPE    1   INT\n");
    fprintf(stream, "MIN_ARG     2\n");
    fprintf(stream, "MAX_ARG     2\n");
    fclose(stream);
}

void test_has_job(const char *job) {
    workflow_joblist_type *joblist =
        (workflow_joblist_type *)workflow_joblist_alloc();

    test_assert_false(workflow_joblist_has_job(joblist, "NoNotThis"));
    test_assert_true(
        workflow_joblist_add_job_from_file(joblist, "CREATE_FILE", job));
    test_assert_true(workflow_joblist_has_job(joblist, "CREATE_FILE"));

    workflow_joblist_free(joblist);
}

int main(int argc, char **argv) {
    const char *exjob_file = "job";
    const char *bin_path = argv[1];
    const char *internal_workflow = argv[2];
    ecl::util::TestArea ta("workflo_test");

    signal(SIGSEGV, util_abort_signal);
    create_exjob(exjob_file, bin_path);
    test_has_job(exjob_file);
    {

        int int_value = rand();
        int read_value = 100;
        workflow_joblist_type *joblist =
            (workflow_joblist_type *)workflow_joblist_alloc();

        if (!workflow_joblist_add_job_from_file(joblist, "CREATE_FILE",
                                                exjob_file)) {
            remove(exjob_file);
            test_error_exit("Loading job CREATE_FILE failed\n");
        } else
            remove(exjob_file);

        if (!workflow_joblist_add_job_from_file(joblist, "READ_FILE",
                                                internal_workflow))
            test_error_exit("Loading job READ_FILE failed\n");

        {
            config_parser_type *workflow_compiler =
                workflow_joblist_get_compiler(joblist);
            if (config_get_schema_size(workflow_compiler) != 2)
                test_error_exit("Config compiler - wrong size \n");
        }

        {
            const char *workflow_file = "workflow";
            const char *tmp_file = "fileX";
            workflow_type *workflow;

            create_workflow(workflow_file, tmp_file, int_value);
            workflow = workflow_alloc(workflow_file, joblist);
            unlink(workflow_file);

            {
                bool runOK;
                runOK = workflow_run(workflow, &read_value, false, NULL);
                if (runOK) {
                    if (int_value != read_value)
                        test_error_exit("Wrong numeric value read back \n");

                    test_assert_int_equal(workflow_get_stack_size(workflow), 2);
                    test_assert_not_NULL(workflow_iget_stack_ptr(workflow, 0));
                    test_assert_NULL(workflow_iget_stack_ptr(workflow, 1));

                    {
                        void *return_value =
                            workflow_iget_stack_ptr(workflow, 0);
                        int return_int = *((int *)return_value);
                        if (int_value != return_int)
                            test_error_exit("Wrong numeric value read back \n");

                        test_assert_not_NULL(workflow_pop_stack(workflow));
                        test_assert_NULL(workflow_pop_stack(workflow));
                        test_assert_int_equal(workflow_get_stack_size(workflow),
                                              0);

                        free(return_value);
                    }
                } else {
                    unlink(tmp_file);
                    test_error_exit("Workflow did not run\n");
                }
                unlink(tmp_file);
            }
        }
        workflow_joblist_free(joblist);
    }
    {
        workflow_joblist_type *joblist = workflow_joblist_alloc();
        const char *workflow_file = "workflow";
        const char *tmp_file = "fileX";
        int read_value;
        int int_value = 100;
        workflow_type *workflow;

        create_workflow(workflow_file, tmp_file, int_value);
        workflow = workflow_alloc(workflow_file, joblist);
        unlink(workflow_file);
        test_assert_false(workflow_run(workflow, &read_value, false, NULL));
        test_assert_int_equal(workflow_get_stack_size(workflow), 0);
    }
    exit(0);
}

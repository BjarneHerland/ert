/*
   Copyright (C) 2015  Equinor ASA, Norway.

   The file 'hook_workflow.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <stdlib.h>
#include <string.h>

#include <ert/util/type_macros.h>
#include <ert/util/util.h>

#include <ert/job_queue/workflow.hpp>

#include <ert/enkf/config_keys.hpp>
#include <ert/enkf/hook_workflow.hpp>

#define RUN_MODE_PRE_FIRST_UPDATE_NAME "PRE_FIRST_UPDATE"
#define RUN_MODE_PRE_SIMULATION_NAME "PRE_SIMULATION"
#define RUN_MODE_POST_SIMULATION_NAME "POST_SIMULATION"
#define RUN_MODE_PRE_UPDATE_NAME "PRE_UPDATE"
#define RUN_MODE_POST_UPDATE_NAME "POST_UPDATE"

#define HOOK_WORKFLOW_TYPE_ID 7321780

struct hook_workflow_struct {
    UTIL_TYPE_ID_DECLARATION;
    hook_run_mode_enum run_mode;
    workflow_type *workflow;
};

static UTIL_SAFE_CAST_FUNCTION(hook_workflow, HOOK_WORKFLOW_TYPE_ID);

hook_workflow_type *hook_workflow_alloc(workflow_type *workflow,
                                        hook_run_mode_enum run_mode) {
    hook_workflow_type *hook_workflow =
        (hook_workflow_type *)util_malloc(sizeof *hook_workflow);
    UTIL_TYPE_ID_INIT(hook_workflow, HOOK_WORKFLOW_TYPE_ID);
    hook_workflow->run_mode = run_mode;
    hook_workflow->workflow = workflow;
    return hook_workflow;
}

void hook_workflow_free(hook_workflow_type *hook_workflow) {
    free(hook_workflow);
}

void hook_workflow_free__(void *arg) {
    hook_workflow_type *hook_workflow = hook_workflow_safe_cast(arg);
    hook_workflow_free(hook_workflow);
}

workflow_type *
hook_workflow_get_workflow(const hook_workflow_type *hook_workflow) {
    return hook_workflow->workflow;
}

bool hook_workflow_run_workflow(const hook_workflow_type *hook_workflow,
                                ert_workflow_list_type *workflow_list,
                                void *self) {
    bool verbose = false;
    if (hook_workflow->workflow != NULL) {
        bool result = ert_workflow_list_run_workflow__(
            workflow_list, hook_workflow->workflow, verbose, self);
        return result;
    } else
        return false;
}

hook_run_mode_enum hook_workflow_run_mode_from_name(const char *run_mode) {
    hook_run_mode_enum mode;
    if (strcmp(run_mode, RUN_MODE_PRE_SIMULATION_NAME) == 0)
        mode = PRE_SIMULATION;
    else if (strcmp(run_mode, RUN_MODE_POST_SIMULATION_NAME) == 0)
        mode = POST_SIMULATION;
    else if (strcmp(run_mode, RUN_MODE_PRE_UPDATE_NAME) == 0)
        mode = PRE_UPDATE;
    else if (strcmp(run_mode, RUN_MODE_POST_UPDATE_NAME) == 0)
        mode = POST_UPDATE;
    else if (strcmp(run_mode, RUN_MODE_PRE_FIRST_UPDATE_NAME) == 0)
        mode = PRE_FIRST_UPDATE;
    else {
        util_abort("%s: unrecognized run mode :%s \n", __func__, run_mode);
        mode = POST_UPDATE; /* Dummy */
    }
    return mode;
}

hook_run_mode_enum
hook_workflow_get_run_mode(const hook_workflow_type *hook_workflow) {
    return hook_workflow->run_mode;
}

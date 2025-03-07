/*
   Copyright (C) 2015  Equinor ASA, Norway.

   The file 'hook_workflow.h' is part of ERT - Ensemble based Reservoir Tool.

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
#ifndef ERT_HOOK_WORKFLOW_H
#define ERT_HOOK_WORKFLOW_H

#include <ert/config/config_parser.hpp>

#include <ert/job_queue/workflow.hpp>

#include <ert/enkf/ert_workflow_list.hpp>

typedef enum {
    PRE_SIMULATION = 0,
    POST_SIMULATION = 1,
    PRE_UPDATE = 2,
    POST_UPDATE = 3,
    PRE_FIRST_UPDATE = 4
} hook_run_mode_enum;

typedef struct hook_workflow_struct hook_workflow_type;

hook_workflow_type *hook_workflow_alloc(workflow_type *workflow,
                                        hook_run_mode_enum run_mode);
void hook_workflow_free(hook_workflow_type *hook_workflow);
void hook_workflow_free__(void *arg);

extern "C" workflow_type *
hook_workflow_get_workflow(const hook_workflow_type *hook_workflow);
bool hook_workflow_run_workflow(const hook_workflow_type *hook_workflow,
                                ert_workflow_list_type *workflow_list,
                                void *self);
extern "C" hook_run_mode_enum
hook_workflow_get_run_mode(const hook_workflow_type *hook_workflow);

hook_run_mode_enum hook_workflow_run_mode_from_name(const char *run_mode);

#endif

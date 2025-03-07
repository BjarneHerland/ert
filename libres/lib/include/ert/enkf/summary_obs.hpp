/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'summary_obs.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_SUMMARY_OBS_H
#define ERT_SUMMARY_OBS_H

#include <stdbool.h>

#include <ert/sched/history.hpp>

#include <ert/config/conf.hpp>

#include <ert/enkf/active_list.hpp>
#include <ert/enkf/enkf_macros.hpp>
#include <ert/enkf/meas_data.hpp>
#include <ert/enkf/obs_data.hpp>
#include <ert/enkf/summary.hpp>
#include <ert/enkf/summary_config.hpp>

typedef struct summary_obs_struct summary_obs_type;

extern "C" void summary_obs_free(summary_obs_type *summary_obs);

extern "C" summary_obs_type *summary_obs_alloc(const char *summary_key,
                                               const char *obs_key,
                                               double value, double std);

extern "C" double summary_obs_get_value(const summary_obs_type *summary_obs);
extern "C" double summary_obs_get_std(const summary_obs_type *summary_obs);
extern "C" double
summary_obs_get_std_scaling(const summary_obs_type *summary_obs);

bool summary_obs_default_used(const summary_obs_type *summary_obs,
                              int restart_nr);

extern "C" PY_USED const char *
summary_obs_get_summary_key(const summary_obs_type *summary_obs);

extern "C" void summary_obs_update_std_scale(summary_obs_type *summary_obs,
                                             double std_multiplier,
                                             const ActiveList *active_list);

extern "C" void summary_obs_set_std_scale(summary_obs_type *summary_obs,
                                          double std_multiplier);

VOID_FREE_HEADER(summary_obs);
VOID_GET_OBS_HEADER(summary_obs);
VOID_MEASURE_HEADER(summary_obs);
UTIL_IS_INSTANCE_HEADER(summary_obs);
VOID_USER_GET_OBS_HEADER(summary_obs);
VOID_CHI2_HEADER(summary_obs);
VOID_UPDATE_STD_SCALE_HEADER(summary_obs);

#endif

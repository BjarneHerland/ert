/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'summary_config.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_SUMMARY_CONFIG_H
#define ERT_SUMMARY_CONFIG_H

#include <stdbool.h>
#include <stdlib.h>

#include <ert/util/type_macros.h>

#include <ert/ecl/ecl_smspec.h>
#include <ert/ecl/ecl_sum.h>

#include <ert/enkf/enkf_macros.hpp>

/**
  How should the run system handle a load problem of a summary
  variable. Observe that the numerical enum values are actually used -
  they should be listed with the most strict mode having the
  numerically largest value.
*/
typedef enum {
    /** We just try to load - and if it is not there we do not care at all. */
    LOAD_FAIL_SILENT = 0,
    /** If the key can not be found we will print a warning on stdout - but the
     * run will still be flagged as successful. */
    LOAD_FAIL_WARN = 2,
    LOAD_FAIL_EXIT = 4
} // The data is deemed important - and we let the run fail if this data can not be found.
load_fail_type;

typedef struct summary_config_struct summary_config_type;
typedef struct summary_struct summary_type;

void summary_config_update_load_fail_mode(summary_config_type *config,
                                          load_fail_type load_fail);
void summary_config_set_load_fail_mode(summary_config_type *config,
                                       load_fail_type load_fail);
load_fail_type
summary_config_get_load_fail_mode(const summary_config_type *config);
extern "C" const char *summary_config_get_var(const summary_config_type *);
extern "C" summary_config_type *summary_config_alloc(const char *,
                                                     load_fail_type load_fail);
extern "C" void summary_config_free(summary_config_type *);

UTIL_IS_INSTANCE_HEADER(summary_config);
UTIL_SAFE_CAST_HEADER(summary_config);
UTIL_SAFE_CAST_HEADER_CONST(summary_config);
GET_DATA_SIZE_HEADER(summary);
VOID_GET_DATA_SIZE_HEADER(summary);
VOID_CONFIG_FREE_HEADER(summary);

#endif

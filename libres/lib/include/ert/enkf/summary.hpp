/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'summary.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_SUMMARY_H
#define ERT_SUMMARY_H
#include <ert/util/double_vector.h>

#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_sum.h>

#include <ert/enkf/enkf_macros.hpp>
#include <ert/enkf/enkf_util.hpp>
#include <ert/enkf/summary_config.hpp>

extern "C" summary_type *
summary_alloc(const summary_config_type *summary_config);
extern "C" void summary_free(summary_type *summary);
extern "C" double summary_get(const summary_type *summary, int report_step);
extern "C" void summary_set(summary_type *summary, int report_step,
                            double value);
bool summary_active_value(double value);
extern "C" int summary_length(const summary_type *summary);
extern "C" double summary_undefined_value();

VOID_HAS_DATA_HEADER(summary);
UTIL_SAFE_CAST_HEADER(summary);
UTIL_SAFE_CAST_HEADER_CONST(summary);
VOID_ALLOC_HEADER(summary);
VOID_FREE_HEADER(summary);
VOID_COPY_HEADER(summary);
VOID_FORWARD_LOAD_HEADER(summary);
VOID_FORWARD_LOAD_VECTOR_HEADER(summary);
VOID_USER_GET_HEADER(summary);
VOID_USER_GET_VECTOR_HEADER(summary);
VOID_WRITE_TO_BUFFER_HEADER(summary);
VOID_READ_FROM_BUFFER_HEADER(summary);
VOID_SERIALIZE_HEADER(summary)
VOID_DESERIALIZE_HEADER(summary)
VOID_CLEAR_HEADER(summary);

#endif

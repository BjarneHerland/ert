/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'trans_func.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_TRANS_FUNC_H
#define ERT_TRANS_FUNC_H
#include <stdbool.h>
#include <stdio.h>

#include <ert/util/double_vector.h>

#include <ert/enkf/enkf_types.hpp>

typedef struct trans_func_struct trans_func_type;
typedef double(transform_ftype)(double, const double_vector_type *);
typedef bool(validate_ftype)(const trans_func_type *);

trans_func_type *trans_func_alloc(const stringlist_type *args);
double trans_func_eval(const trans_func_type *trans_func, double x);

void trans_func_free(trans_func_type *trans_func);
bool trans_func_use_log_scale(const trans_func_type *trans_func);
stringlist_type *trans_func_get_param_names(const trans_func_type *trans_func);
double_vector_type *trans_func_get_params(const trans_func_type *trans_func);
const char *trans_func_get_name(const trans_func_type *trans_func);

#endif

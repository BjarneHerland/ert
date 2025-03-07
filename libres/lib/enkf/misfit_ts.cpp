/*
   Copyright (C) 2012  Equinor ASA, Norway.

   The file 'misfit_ts.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/util/double_vector.h>
#include <ert/util/int_vector.h>
#include <ert/util/type_macros.h>
#include <ert/util/util.h>

#include <ert/enkf/misfit_ts.hpp>

#define MISFIT_TS_TYPE_ID 641066

/** misfit for one ensemble member / observation key. */
struct misfit_ts_struct {
    UTIL_TYPE_ID_DECLARATION;
    /** A double vector of length 'history_length' with actual misfit values. */
    double_vector_type *data;
};

static UTIL_SAFE_CAST_FUNCTION(misfit_ts, MISFIT_TS_TYPE_ID);

misfit_ts_type *misfit_ts_alloc(int history_length) {
    misfit_ts_type *misfit_ts =
        (misfit_ts_type *)util_malloc(sizeof *misfit_ts);
    UTIL_TYPE_ID_INIT(misfit_ts, MISFIT_TS_TYPE_ID);

    if (history_length > 0)
        misfit_ts->data = double_vector_alloc(history_length + 1, 0);
    else
        misfit_ts->data =
            NULL; /* Used by the xxx_fread_alloc() function below. */

    return misfit_ts;
}

misfit_ts_type *misfit_ts_fread_alloc(FILE *stream) {
    misfit_ts_type *misfit_ts = misfit_ts_alloc(0);
    if (misfit_ts->data == NULL)
        misfit_ts->data = double_vector_fread_alloc(stream);
    return misfit_ts;
}

void misfit_ts_fwrite(const misfit_ts_type *misfit_ts, FILE *stream) {
    double_vector_fwrite(misfit_ts->data, stream);
}

static void misfit_ts_free(misfit_ts_type *misfit_ts) {
    double_vector_free(misfit_ts->data);
    free(misfit_ts);
}

void misfit_ts_free__(void *vector) {
    misfit_ts_free(misfit_ts_safe_cast(vector));
}

void misfit_ts_iset(misfit_ts_type *vector, int time_index, double value) {
    double_vector_iset(vector->data, time_index, value);
}

/* Step2 is inclusive */
double misfit_ts_eval(const misfit_ts_type *vector,
                      const int_vector_type *steps) {
    double misfit_sum = 0;
    int step;

    for (int i = 0; i < int_vector_size(steps); ++i) {
        step = int_vector_iget(steps, i);
        misfit_sum += double_vector_iget(vector->data, step);
    }

    return misfit_sum;
}

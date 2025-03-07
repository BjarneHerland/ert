/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'trans_func.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <cmath>
#include <stdio.h>
#include <stdlib.h>

#include <ert/util/double_vector.h>
#include <ert/util/stringlist.h>
#include <ert/util/util.h>

#include <ert/enkf/trans_func.hpp>

struct trans_func_struct {
    /** The name this function is registered as. */
    char *name;
    /** The parameter values registered for this function. */
    double_vector_type *params;
    /** A pointer to the actual transformation function. */
    transform_ftype *func;
    /** A pointer to a a function which can be used to validate the parameters can be NULL. */
    validate_ftype *validate;
    /* A list of the parameter names. */
    stringlist_type *param_names;
    bool use_log;
};

/*
   Width  = 1 => uniform
   Width  > 1 => unimodal peaked
   Width  < 1 => bimoal peaks


   Skewness < 0 => shifts towards the left
   Skewness = 0 => symmetric
   Skewness > 0 => Shifts towards the right

   The width is a relavant scale for the value of skewness.
*/

static double trans_errf(double x, const double_vector_type *arg) {
    double min = double_vector_iget(arg, 0);
    double max = double_vector_iget(arg, 1);
    double skewness = double_vector_iget(arg, 2);
    double width = double_vector_iget(arg, 3);
    double y;

    y = 0.5 * (1 + erf((x + skewness) / (width * sqrt(2.0))));
    return min + y * (max - min);
}

static double trans_const(double x, const double_vector_type *arg) {
    return double_vector_iget(arg, 0);
}

static double trans_raw(double x, const double_vector_type *arg) { return x; }

/* Observe that the argument of the shift should be "+" */
static double trans_derrf(double x, const double_vector_type *arg) {
    int steps = double_vector_iget(arg, 0);
    double min = double_vector_iget(arg, 1);
    double max = double_vector_iget(arg, 2);
    double skewness = double_vector_iget(arg, 3);
    double width = double_vector_iget(arg, 4);
    double y;

    y = floor(steps * 0.5 * (1 + erf((x + skewness) / (width * sqrt(2.0)))) /
              (steps - 1));
    return min + y * (max - min);
}

static double trans_unif(double x, const double_vector_type *arg) {
    double y;
    double min = double_vector_iget(arg, 0);
    double max = double_vector_iget(arg, 1);
    y = 0.5 * (1 + erf(x / sqrt(2.0))); /* 0 - 1 */
    return y * (max - min) + min;
}

static double trans_dunif(double x, const double_vector_type *arg) {
    double y;
    int steps = double_vector_iget(arg, 0);
    double min = double_vector_iget(arg, 1);
    double max = double_vector_iget(arg, 2);

    y = 0.5 * (1 + erf(x / sqrt(2.0))); /* 0 - 1 */
    return (floor(y * steps) / (steps - 1)) * (max - min) + min;
}

static double trans_normal(double x, const double_vector_type *arg) {
    double mu, std;
    mu = double_vector_iget(arg, 0);
    std = double_vector_iget(arg, 1);
    return x * std + mu;
}

static double trans_truncated_normal(double x, const double_vector_type *arg) {
    double mu, std, min, max;

    mu = double_vector_iget(arg, 0);
    std = double_vector_iget(arg, 1);
    min = double_vector_iget(arg, 2);
    max = double_vector_iget(arg, 3);

    {
        double y = x * std + mu;
        util_clamp_double(&y, min, max);
        return y;
    }
}

static double trans_lognormal(double x, const double_vector_type *arg) {
    double mu, std;
    mu = double_vector_iget(arg, 0); /* The expectation of log( y ) */
    std = double_vector_iget(arg, 1);
    return exp(x * std + mu);
}

/**
   Used to sample values between min and max - BUT it is the logarithm
   of y which is uniformly distributed. Relates to the uniform
   distribution in the same manner as the lognormal distribution
   relates to the normal distribution.
*/
static double trans_logunif(double x, const double_vector_type *arg) {
    double log_min = log(double_vector_iget(arg, 0));
    double log_max = log(double_vector_iget(arg, 1));
    double log_y;
    {
        double tmp = 0.5 * (1 + erf(x / sqrt(2.0))); /* 0 - 1 */
        log_y = log_min +
                tmp * (log_max - log_min); /* Shift according to max / min */
    }
    return exp(log_y);
}

static double trans_triangular(double x, const double_vector_type *arg) {
    double xmin = double_vector_iget(arg, 0);
    double xmode = double_vector_iget(arg, 1);
    double xmax = double_vector_iget(arg, 2);

    double inv_norm_left = (xmax - xmin) * (xmode - xmin);
    double inv_norm_right = (xmax - xmin) * (xmax - xmode);
    double ymode = (xmode - xmin) / (xmax - xmin);
    double y = 0.5 * (1 + erf(x / sqrt(2.0))); /* 0 - 1 */

    if (y < ymode)
        return xmin + sqrt(y * inv_norm_left);
    else
        return xmax - sqrt((1 - y) * inv_norm_right);
}

void trans_func_free(trans_func_type *trans_func) {
    stringlist_free(trans_func->param_names);
    double_vector_free(trans_func->params);
    free(trans_func->name);
    free(trans_func);
}

static trans_func_type *trans_func_alloc_empty(const char *func_name) {
    trans_func_type *trans_func =
        (trans_func_type *)util_malloc(sizeof *trans_func);

    trans_func->params = double_vector_alloc(0, 0);
    trans_func->func = NULL;
    trans_func->validate = NULL;
    trans_func->name = util_alloc_string_copy(func_name);
    trans_func->param_names = stringlist_alloc_new();
    trans_func->use_log = false;

    return trans_func;
}

trans_func_type *trans_func_alloc(const stringlist_type *args) {
    const char *func_name = stringlist_iget(args, 0);
    trans_func_type *trans_func = trans_func_alloc_empty(func_name);

    if (util_string_equal(func_name, "NORMAL")) {
        stringlist_append_copy(trans_func->param_names, "MEAN");
        stringlist_append_copy(trans_func->param_names, "STD");
        trans_func->func = trans_normal;
    }

    if (util_string_equal(func_name, "LOGNORMAL")) {
        stringlist_append_copy(trans_func->param_names, "MEAN");
        stringlist_append_copy(trans_func->param_names, "STD");
        trans_func->func = trans_lognormal;
        trans_func->use_log = true;
    }

    if (util_string_equal(func_name, "TRUNCATED_NORMAL")) {
        stringlist_append_copy(trans_func->param_names, "MEAN");
        stringlist_append_copy(trans_func->param_names, "STD");
        stringlist_append_copy(trans_func->param_names, "MIN");
        stringlist_append_copy(trans_func->param_names, "MAX");

        trans_func->func = trans_truncated_normal;
    }

    if (util_string_equal(func_name, "TRIANGULAR")) {
        stringlist_append_copy(trans_func->param_names, "XMIN");
        stringlist_append_copy(trans_func->param_names, "XMODE");
        stringlist_append_copy(trans_func->param_names, "XMAX");

        trans_func->func = trans_triangular;
    }

    if (util_string_equal(func_name, "UNIFORM")) {
        stringlist_append_copy(trans_func->param_names, "MIN");
        stringlist_append_copy(trans_func->param_names, "MAX");
        trans_func->func = trans_unif;
    }

    if (util_string_equal(func_name, "DUNIF")) {
        stringlist_append_copy(trans_func->param_names, "STEPS");
        stringlist_append_copy(trans_func->param_names, "MIN");
        stringlist_append_copy(trans_func->param_names, "MAX");

        trans_func->func = trans_dunif;
    }

    if (util_string_equal(func_name, "ERRF")) {
        stringlist_append_copy(trans_func->param_names, "MIN");
        stringlist_append_copy(trans_func->param_names, "MAX");
        stringlist_append_copy(trans_func->param_names, "SKEWNESS");
        stringlist_append_copy(trans_func->param_names, "WIDTH");

        trans_func->func = trans_errf;
    }

    if (util_string_equal(func_name, "DERRF")) {
        stringlist_append_copy(trans_func->param_names, "STEPS");
        stringlist_append_copy(trans_func->param_names, "MIN");
        stringlist_append_copy(trans_func->param_names, "MAX");
        stringlist_append_copy(trans_func->param_names, "SKEWNESS");
        stringlist_append_copy(trans_func->param_names, "WIDTH");

        trans_func->func = trans_derrf;
    }

    if (util_string_equal(func_name, "LOGUNIF")) {
        stringlist_append_copy(trans_func->param_names, "MIN");
        stringlist_append_copy(trans_func->param_names, "MAX");

        trans_func->func = trans_logunif;
        trans_func->use_log = true;
    }

    if (util_string_equal(func_name, "CONST")) {
        stringlist_append_copy(trans_func->param_names, "VALUE");
        trans_func->func = trans_const;
    }

    if (util_string_equal(func_name, "RAW"))
        trans_func->func = trans_raw;

    /* Parsing parameter values. */

    if (!trans_func->func) {
        trans_func_free(trans_func);
        return NULL;
    }

    if (stringlist_get_size(args) -
            stringlist_get_size(trans_func->param_names) !=
        1) {
        trans_func_free(trans_func);
        return NULL;
    }

    for (int iarg = 0; iarg < stringlist_get_size(trans_func->param_names);
         iarg++) {
        double param_value;

        if (util_sscanf_double(stringlist_iget(args, iarg + 1), &param_value))
            double_vector_append(trans_func->params, param_value);
        else {
            fprintf(stderr, "%s: could not parse: %s as floating point value\n",
                    __func__, stringlist_iget(args, iarg + 1));
            trans_func_free(trans_func);
            return NULL;
        }
    }

    return trans_func;
}

double trans_func_eval(const trans_func_type *trans_func, double x) {
    double y = trans_func->func(x, trans_func->params);
    return y;
}

bool trans_func_use_log_scale(const trans_func_type *trans_func) {
    return trans_func->use_log;
}

stringlist_type *trans_func_get_param_names(const trans_func_type *trans_func) {
    return trans_func->param_names;
}
double_vector_type *trans_func_get_params(const trans_func_type *trans_func) {
    return trans_func->params;
}
const char *trans_func_get_name(const trans_func_type *trans_func) {
    return trans_func->name;
}

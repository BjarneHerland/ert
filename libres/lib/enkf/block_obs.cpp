/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'block_obs.c' is part of ERT - Ensemble based Reservoir Tool.

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

/*
   See the overview documentation of the observation system in enkf_obs.c
*/
#include <stdlib.h>

#include <ert/util/stringlist.h>
#include <ert/util/util.h>
#include <ert/util/vector.h>

#include <ert/ecl/ecl_grid.h>

#include "ert/python.hpp"
#include <ert/enkf/block_obs.hpp>
#include <ert/enkf/container.hpp>
#include <ert/enkf/container_config.hpp>
#include <ert/enkf/enkf_util.hpp>
#include <ert/enkf/field.hpp>
#include <ert/enkf/obs_data.hpp>
#include <ert/enkf/summary.hpp>

#define BLOCK_OBS_TYPE_ID 661098
#define POINT_OBS_TYPE_ID 778196

typedef struct {
    UTIL_TYPE_ID_DECLARATION;
    block_obs_source_type source_type;
    int i;
    int j;
    int k;
    int active_index;
    double value;
    double std;
    double std_scaling;
    char *sum_key;
} point_obs_type;

static UTIL_SAFE_CAST_FUNCTION(point_obs, POINT_OBS_TYPE_ID);

struct block_obs_struct {
    UTIL_TYPE_ID_DECLARATION;
    /** A user provided label for the observation.*/
    char *obs_key;
    vector_type *point_list;
    const ecl_grid_type *grid;
    const void *data_config;
    block_obs_source_type source_type;
};

static UTIL_SAFE_CAST_FUNCTION_CONST(block_obs, BLOCK_OBS_TYPE_ID);
static UTIL_SAFE_CAST_FUNCTION(block_obs, BLOCK_OBS_TYPE_ID);
UTIL_IS_INSTANCE_FUNCTION(block_obs, BLOCK_OBS_TYPE_ID);

static point_obs_type *point_obs_alloc(block_obs_source_type source_type, int i,
                                       int j, int k, int active_index,
                                       const char *sum_key, double value,
                                       double std) {
    point_obs_type *point_obs =
        (point_obs_type *)util_malloc(sizeof *point_obs);
    UTIL_TYPE_ID_INIT(point_obs, POINT_OBS_TYPE_ID);

    if (source_type == SOURCE_FIELD) {
        point_obs->active_index = active_index;
        point_obs->sum_key = NULL;
    } else {
        point_obs->active_index = -1;
        point_obs->sum_key = util_alloc_string_copy(sum_key);
    }

    point_obs->source_type = source_type;
    point_obs->i = i;
    point_obs->j = j;
    point_obs->k = k;
    point_obs->value = value;
    point_obs->std = std;
    point_obs->std_scaling = 1.0;

    return point_obs;
}

static void point_obs_free(point_obs_type *point_obs) {
    free(point_obs->sum_key);
    free(point_obs);
}

static void point_obs_free__(void *arg) {
    point_obs_type *point_obs = point_obs_safe_cast(arg);
    point_obs_free(point_obs);
}

static double point_obs_iget_data(const point_obs_type *point_obs,
                                  const void *state, int iobs,
                                  node_id_type node_id) {
    if (point_obs->source_type == SOURCE_FIELD) {
        const field_type *field = field_safe_cast_const(state);
        return field_iget_double(field, point_obs->active_index);
    } else if (point_obs->source_type == SOURCE_SUMMARY) {
        const container_type *container = container_safe_cast_const(state);
        const summary_type *summary =
            summary_safe_cast_const(container_iget_node(container, iobs));
        return summary_get(summary, node_id.report_step);
    } else {
        util_abort("%s: unknown source type: %d \n", __func__,
                   point_obs->source_type);
        return -1;
    }
}

static const point_obs_type *
block_obs_iget_point_const(const block_obs_type *block_obs, int index) {
    return (const point_obs_type *)vector_iget_const(block_obs->point_list,
                                                     index);
}

static point_obs_type *block_obs_iget_point(const block_obs_type *block_obs,
                                            int index) {
    return (point_obs_type *)vector_iget(block_obs->point_list, index);
}

static void block_obs_validate_ijk(const ecl_grid_type *grid, int size,
                                   const int *i, const int *j, const int *k) {
    int l;
    for (l = 0; l < size; l++) {
        if (ecl_grid_ijk_valid(grid, i[l], j[l], k[l])) {
            int active_index =
                ecl_grid_get_active_index3(grid, i[l], j[l], k[l]);
            if (active_index < 0)
                util_abort("%s: sorry: cell:(%d,%d,%d) is not active - can not "
                           "observe it. \n",
                           __func__, i[l] + 1, j[l] + 1, k[l] + 1);

        } else
            util_abort("%s: sorry: cell (%d,%d,%d) is outside valid range:  \n",
                       __func__, i[l] + 1, j[l] + 1, k[l] + 1);
    }
}

static void block_obs_append_point(block_obs_type *block_obs,
                                   point_obs_type *point) {
    if (point->source_type == block_obs->source_type)
        vector_append_owned_ref(block_obs->point_list, point, point_obs_free__);
    else
        util_abort("%s: fatal internal error - mixing points with different "
                   "source type in one block_obs instance.\n",
                   __func__);
}

void block_obs_append_field_obs(block_obs_type *block_obs, int i, int j, int k,
                                double value, double std) {
    int active_index = ecl_grid_get_active_index3(block_obs->grid, i, j, k);
    point_obs_type *point_obs =
        point_obs_alloc(SOURCE_FIELD, i, j, k, active_index, NULL, value, std);
    block_obs_append_point(block_obs, point_obs);
}

void block_obs_append_summary_obs(block_obs_type *block_obs, int i, int j,
                                  int k, const char *sum_key, double value,
                                  double std) {
    int active_index = -1;
    point_obs_type *point_obs = point_obs_alloc(
        SOURCE_SUMMARY, i, j, k, active_index, sum_key, value, std);
    block_obs_append_point(block_obs, point_obs);
}

block_obs_type *block_obs_alloc(const char *obs_key, const void *data_config,
                                const ecl_grid_type *grid) {

    if (!(field_config_is_instance(data_config) ||
          container_config_is_instance(data_config)))
        return NULL;

    {
        block_obs_type *block_obs =
            (block_obs_type *)util_malloc(sizeof *block_obs);
        UTIL_TYPE_ID_INIT(block_obs, BLOCK_OBS_TYPE_ID);

        block_obs->obs_key = util_alloc_string_copy(obs_key);
        block_obs->data_config = data_config;
        block_obs->point_list = vector_alloc_new();
        block_obs->grid = grid;

        if (field_config_is_instance(data_config))
            block_obs->source_type = SOURCE_FIELD;
        else
            block_obs->source_type = SOURCE_SUMMARY;

        return block_obs;
    }
}

/**
   The input vectors i,j,k should contain offset zero values.
*/
block_obs_type *
block_obs_alloc_complete(const char *obs_key, block_obs_source_type source_type,
                         const stringlist_type *summary_keys,
                         const void *data_config, const ecl_grid_type *grid,
                         int size, const int *i, const int *j, const int *k,
                         const double *obs_value, const double *obs_std) {
    if (source_type == SOURCE_FIELD)
        block_obs_validate_ijk(grid, size, i, j, k);

    {
        block_obs_type *block_obs = block_obs_alloc(obs_key, data_config, grid);
        if (block_obs) {
            for (int l = 0; l < size; l++) {

                if (source_type == SOURCE_SUMMARY) {
                    const char *sum_key = stringlist_iget(summary_keys, l);
                    block_obs_append_summary_obs(block_obs, i[l], j[l], k[l],
                                                 sum_key, obs_value[l],
                                                 obs_std[l]);
                } else
                    block_obs_append_field_obs(block_obs, i[l], j[l], k[l],
                                               obs_value[l], obs_std[l]);
            }
            return block_obs;
        } else {
            util_abort(
                "%s: internal error - block_obs_alloc() returned NULL \n",
                __func__);
            return NULL;
        }
    }
}

void block_obs_free(block_obs_type *block_obs) {
    vector_free(block_obs->point_list);
    free(block_obs->obs_key);
    free(block_obs);
}

void block_obs_get_observations(const block_obs_type *block_obs,
                                obs_data_type *obs_data, enkf_fs_type *fs,
                                int report_step) {
    int obs_size = block_obs_get_size(block_obs);
    obs_block_type *obs_block =
        obs_data_add_block(obs_data, block_obs->obs_key, obs_size);

    for (int i = 0; i < obs_size; i++) {
        const point_obs_type *point_obs =
            block_obs_iget_point_const(block_obs, i);
        obs_block_iset(obs_block, i, point_obs->value,
                       point_obs->std * point_obs->std_scaling);
    }
}

static void block_obs_assert_data(const block_obs_type *block_obs,
                                  const void *state) {
    if (block_obs->source_type == SOURCE_FIELD) {
        if (!field_is_instance(state))
            util_abort("%s: state data is not of type FIELD - aborting \n",
                       __func__);
    } else if (block_obs->source_type == SOURCE_SUMMARY) {
        if (!container_is_instance(state))
            util_abort("%s: state data is not of type CONTAINER - aborting \n",
                       __func__);
    }
}

double block_obs_iget_data(const block_obs_type *block_obs, const void *state,
                           int iobs, node_id_type node_id) {
    const point_obs_type *point_obs =
        block_obs_iget_point_const(block_obs, iobs);
    return point_obs_iget_data(point_obs, state, iobs, node_id);
}

void block_obs_measure(const block_obs_type *block_obs, const void *state,
                       node_id_type node_id, meas_data_type *meas_data) {
    block_obs_assert_data(block_obs, state);
    int obs_size = block_obs_get_size(block_obs);
    meas_block_type *meas_block = meas_data_add_block(
        meas_data, block_obs->obs_key, node_id.report_step, obs_size);

    for (int iobs = 0; iobs < obs_size; iobs++) {
        double value = block_obs_iget_data(block_obs, state, iobs, node_id);
        meas_block_iset(meas_block, node_id.iens, iobs, value);
    }
}

// used by the VOID_CHI2 macro
C_USED double block_obs_chi2(const block_obs_type *block_obs, const void *state,
                             node_id_type node_id) {
    double sum_chi2 = 0;
    int obs_size = block_obs_get_size(block_obs);
    block_obs_assert_data(block_obs, state);

    for (int i = 0; i < obs_size; i++) {
        const point_obs_type *point_obs =
            block_obs_iget_point_const(block_obs, i);
        double sim_value = point_obs_iget_data(point_obs, state, i, node_id);
        double x = (sim_value - point_obs->value) / point_obs->std;
        sum_chi2 += x * x;
    }
    return sum_chi2;
}

double block_obs_iget_value(const block_obs_type *block_obs, int index) {
    const point_obs_type *point_obs =
        block_obs_iget_point_const(block_obs, index);
    return point_obs->value;
}

double block_obs_iget_std(const block_obs_type *block_obs, int index) {
    const point_obs_type *point_obs =
        block_obs_iget_point_const(block_obs, index);
    return point_obs->std;
}

double block_obs_iget_std_scaling(const block_obs_type *block_obs, int index) {
    const point_obs_type *point_obs =
        block_obs_iget_point_const(block_obs, index);
    return point_obs->std_scaling;
}

void block_obs_user_get(const block_obs_type *block_obs, const char *index_key,
                        double *value, double *std, bool *valid) {
    int i, j, k;

    *valid = false;
    if (field_config_parse_user_key__(index_key, &i, &j, &k)) {
        int obs_size = block_obs_get_size(block_obs);
        int active_index = ecl_grid_get_active_index3(block_obs->grid, i, j, k);
        int l = 0;
        /* iterating through all the cells the observation is observing. */

        while (!(*valid) && l < obs_size) {
            const point_obs_type *point_obs =
                block_obs_iget_point_const(block_obs, l);
            if (point_obs->active_index == active_index) {
                *value = point_obs->value;
                *std = point_obs->std;
                *valid = true;
            }
            l++;
        }
    }
}

int block_obs_iget_i(const block_obs_type *block_obs, int index) {
    const point_obs_type *point_obs =
        block_obs_iget_point_const(block_obs, index);
    return point_obs->i;
}

int block_obs_iget_j(const block_obs_type *block_obs, int index) {
    const point_obs_type *point_obs =
        block_obs_iget_point_const(block_obs, index);
    return point_obs->j;
}

int block_obs_iget_k(const block_obs_type *block_obs, int index) {
    const point_obs_type *point_obs =
        block_obs_iget_point_const(block_obs, index);
    return point_obs->k;
}

double block_obs_iget_depth(const block_obs_type *block_obs, int index) {
    const point_obs_type *point_obs =
        block_obs_iget_point_const(block_obs, index);
    return ecl_grid_get_cdepth3(block_obs->grid, point_obs->i, point_obs->j,
                                point_obs->k);
}

int block_obs_get_size(const block_obs_type *block_obs) {
    return vector_get_size(block_obs->point_list);
}

void block_obs_update_std_scale(block_obs_type *block_obs, double scale_factor,
                                const ActiveList *active_list) {
    int obs_size = block_obs_get_size(block_obs);
    if (active_list->getMode() == ALL_ACTIVE) {
        for (int i = 0; i < obs_size; i++) {
            point_obs_type *point_observation =
                block_obs_iget_point(block_obs, i);
            point_observation->std_scaling = scale_factor;
        }
    } else {
        const int *active_index = active_list->active_list_get_active();
        int size = active_list->active_size(obs_size);
        for (int i = 0; i < size; i++) {
            int obs_index = active_index[i];
            point_obs_type *point_observation =
                block_obs_iget_point(block_obs, obs_index);
            point_observation->std_scaling = scale_factor;
        }
    }
}

VOID_FREE(block_obs)
VOID_GET_OBS(block_obs)
VOID_MEASURE_UNSAFE(
    block_obs,
    data) // The cast of data field is not checked - that is done in block_obs_measure().
VOID_USER_GET_OBS(block_obs)
VOID_CHI2(block_obs, field)
VOID_UPDATE_STD_SCALE(block_obs)

class ActiveList;
namespace {
void update_std_scaling(py::handle obj, double scaling,
                        const ActiveList &active_list) {
    auto *self = ert::from_cwrap<block_obs_type>(obj);
    block_obs_update_std_scale(self, scaling, &active_list);
}
} // namespace

RES_LIB_SUBMODULE("local.block_obs", m) {
    using namespace py::literals;

    m.def("update_std_scaling", &update_std_scaling, "self"_a, "scaling"_a,
          "active_list"_a);
}

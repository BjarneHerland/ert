/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'enkf_macros.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_ENKF_MACROS_H
#define ERT_ENKF_MACROS_H

#include <Eigen/Dense>
#include <stdio.h>
#include <stdlib.h>

#include <ert/util/double_vector.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/rng.hpp>

#include <ert/ecl/ecl_file.hpp>
#include <ert/ecl/ecl_sum.hpp>

#include <ert/enkf/active_list.hpp>
#include <ert/enkf/enkf_fs_type.hpp>
#include <ert/enkf/enkf_serialize.hpp>
#include <ert/enkf/enkf_types.hpp>
#include <ert/enkf/forward_load_context.hpp>
#include <ert/enkf/meas_data.hpp>
#include <ert/enkf/value_export.hpp>

#define VOID_CONFIG_FREE(prefix)                                               \
    void prefix##_config_free__(void *void_arg) {                              \
        prefix##_config_free((prefix##_config_type *)void_arg);                \
    }
#define VOID_CONFIG_FREE_HEADER(prefix) void prefix##_config_free__(void *);

#define GET_DATA_SIZE(prefix)                                                  \
    int prefix##_config_get_data_size(const prefix##_config_type *arg) {       \
        return arg->data_size;                                                 \
    }
#define GET_DATA_SIZE_HEADER(prefix)                                           \
    int prefix##_config_get_data_size(const prefix##_config_type *arg);

#define VOID_GET_DATA_SIZE(prefix)                                             \
    int prefix##_config_get_data_size__(const void *arg) {                     \
        const prefix##_config_type *config =                                   \
            prefix##_config_safe_cast_const(arg);                              \
        return prefix##_config_get_data_size(config);                          \
    }
#define VOID_GET_DATA_SIZE_HEADER(prefix)                                      \
    int prefix##_config_get_data_size__(const void *arg);

#define VOID_ALLOC(prefix)                                                     \
    void *prefix##_alloc__(const void *void_config) {                          \
        const prefix##_config_type *config =                                   \
            prefix##_config_safe_cast_const(void_config);                      \
        return prefix##_alloc(config);                                         \
    }

#define VOID_ALLOC_HEADER(prefix) void *prefix##_alloc__(const void *);

#define VOID_HAS_DATA(prefix)                                                  \
    bool prefix##_has_data__(const void *void_arg, int report_step) {          \
        const prefix##_type *arg = prefix##_safe_cast_const(void_arg);         \
        return prefix##_has_data(arg, report_step);                            \
    }

#define VOID_HAS_DATA_HEADER(prefix)                                           \
    bool prefix##_has_data__(const void *, int);

#define VOID_WRITE_TO_BUFFER(prefix)                                           \
    bool prefix##_write_to_buffer__(const void *void_arg, buffer_type *buffer, \
                                    int report_step) {                         \
        const prefix##_type *arg = prefix##_safe_cast_const(void_arg);         \
        return prefix##_write_to_buffer(arg, buffer, report_step);             \
    }

#define VOID_READ_FROM_BUFFER(prefix)                                          \
    void prefix##_read_from_buffer__(void *void_arg, buffer_type *buffer,      \
                                     enkf_fs_type *fs, int report_step) {      \
        prefix##_type *arg = prefix##_safe_cast(void_arg);                     \
        prefix##_read_from_buffer(arg, buffer, fs, report_step);               \
    }

#define VOID_WRITE_TO_BUFFER_HEADER(prefix)                                    \
    bool prefix##_write_to_buffer__(const void *, buffer_type *, int);
#define VOID_READ_FROM_BUFFER_HEADER(prefix)                                   \
    void prefix##_read_from_buffer__(void *, buffer_type *, enkf_fs_type *,    \
                                     int);

#define VOID_FLOAD(prefix)                                                     \
    bool prefix##_fload__(void *void_arg, const char *filename) {              \
        prefix##_type *arg = prefix##_safe_cast(void_arg);                     \
        return prefix##_fload(arg, filename);                                  \
    }
#define VOID_FLOAD_HEADER(prefix) bool prefix##_fload__(void *, const char *);

#define VOID_ECL_WRITE(prefix)                                                 \
    void prefix##_ecl_write__(const void *void_arg, const char *path,          \
                              const char *file,                                \
                              value_export_type *export_value) {               \
        const prefix##_type *arg = prefix##_safe_cast_const(void_arg);         \
        prefix##_ecl_write(arg, path, file, export_value);                     \
    }

#define VOID_ECL_WRITE_HEADER(prefix)                                          \
    void prefix##_ecl_write__(const void *, const char *, const char *,        \
                              value_export_type *export_value);

#define VOID_FORWARD_LOAD(prefix)                                              \
    bool prefix##_forward_load__(                                              \
        void *void_arg, const char *ecl_file,                                  \
        const forward_load_context_type *load_context) {                       \
        prefix##_type *arg = prefix##_safe_cast(void_arg);                     \
        return prefix##_forward_load(arg, ecl_file, load_context);             \
    }

#define VOID_FORWARD_LOAD_HEADER(prefix)                                       \
    bool prefix##_forward_load__(                                              \
        void *, const char *, const forward_load_context_type *load_context);

#define VOID_FORWARD_LOAD_VECTOR(prefix)                                       \
    bool prefix##_forward_load_vector__(                                       \
        void *void_arg, const char *ecl_file,                                  \
        const forward_load_context_type *load_context,                         \
        const int_vector_type *time_index) {                                   \
        prefix##_type *arg = prefix##_safe_cast(void_arg);                     \
        return prefix##_forward_load_vector(arg, ecl_file, load_context,       \
                                            time_index);                       \
    }

#define VOID_FORWARD_LOAD_VECTOR_HEADER(prefix)                                \
    bool prefix##_forward_load_vector__(                                       \
        void *, const char *, const forward_load_context_type *load_context,   \
        const int_vector_type *time_index);

#define VOID_FREE(prefix)                                                      \
    void prefix##_free__(void *void_arg) {                                     \
        prefix##_type *arg = prefix##_safe_cast(void_arg);                     \
        prefix##_free(arg);                                                    \
    }

#define VOID_FREE_HEADER(prefix) void prefix##_free__(void *);

#define VOID_USER_GET(prefix)                                                  \
    bool prefix##_user_get__(void *void_arg, const char *key, int report_step, \
                             double *value) {                                  \
        prefix##_type *arg = prefix##_safe_cast(void_arg);                     \
        return prefix##_user_get(arg, key, report_step, value);                \
    }

#define VOID_USER_GET_HEADER(prefix)                                           \
    bool prefix##_user_get__(void *, const char *, int, double *);

#define VOID_USER_GET_VECTOR(prefix)                                           \
    void prefix##_user_get_vector__(void *void_arg, const char *key,           \
                                    double_vector_type *value) {               \
        prefix##_type *arg = prefix##_safe_cast(void_arg);                     \
        prefix##_user_get_vector(arg, key, value);                             \
    }

#define VOID_USER_GET_VECTOR_HEADER(prefix)                                    \
    void prefix##_user_get_vector__(void *, const char *, double_vector_type *);

#define VOID_USER_GET_OBS(prefix)                                              \
    void prefix##_user_get__(void *void_arg, const char *key, double *value,   \
                             double *std, bool *valid) {                       \
        prefix##_user_get((prefix##_type *)void_arg, key, value, std, valid);  \
    }

#define VOID_USER_GET_OBS_HEADER(prefix)                                       \
    void prefix##_user_get__(void *, const char *, double *, double *, bool *);

#define VOID_COPY(prefix)                                                      \
    void prefix##_copy__(const void *void_src, void *void_target) {            \
        const prefix##_type *src = prefix##_safe_cast_const(void_src);         \
        prefix##_type *target = prefix##_safe_cast(void_target);               \
        prefix##_copy(src, target);                                            \
    }
#define VOID_COPY_HEADER(prefix) void prefix##_copy__(const void *, void *);

#define CONFIG_GET_ECL_KW_NAME(prefix)                                         \
    const char *prefix##_config_get_ecl_kw_name(                               \
        const prefix##_config_type *config) {                                  \
        return config->ecl_kw_name;                                            \
    }
#define CONFIG_GET_ECL_KW_NAME_HEADER(prefix)                                  \
    const char *prefix##_config_get_ecl_kw_name(const prefix##_config_type *)

#define VOID_SERIALIZE(prefix)                                                 \
    void prefix##_serialize__(const void *void_arg, node_id_type node_id,      \
                              const ActiveList *active_list,                   \
                              Eigen::MatrixXd &A, int row_offset,              \
                              int column) {                                    \
        const prefix##_type *arg = prefix##_safe_cast_const(void_arg);         \
        prefix##_serialize(arg, node_id, active_list, A, row_offset, column);  \
    }
#define VOID_SERIALIZE_HEADER(prefix)                                          \
    void prefix##_serialize__(const void *, node_id_type, const ActiveList *,  \
                              Eigen::MatrixXd &, int, int);

#define VOID_DESERIALIZE(prefix)                                               \
    void prefix##_deserialize__(                                               \
        void *void_arg, node_id_type node_id, const ActiveList *active_list,   \
        const Eigen::MatrixXd &A, int row_offset, int column) {                \
        prefix##_type *arg = prefix##_safe_cast(void_arg);                     \
        prefix##_deserialize(arg, node_id, active_list, A, row_offset,         \
                             column);                                          \
    }
#define VOID_DESERIALIZE_HEADER(prefix)                                        \
    void prefix##_deserialize__(void *, node_id_type, const ActiveList *,      \
                                const Eigen::MatrixXd &, int, int);

#define VOID_INITIALIZE(prefix)                                                \
    bool prefix##_initialize__(void *void_arg, int iens,                       \
                               const char *init_file, rng_type *rng) {         \
        prefix##_type *arg = prefix##_safe_cast(void_arg);                     \
        return prefix##_initialize(arg, iens, init_file, rng);                 \
    }
#define VOID_INITIALIZE_HEADER(prefix)                                         \
    bool prefix##_initialize__(void *, int, const char *, rng_type *);

#define VOID_GET_OBS(prefix)                                                   \
    void prefix##_get_observations__(const void *void_arg,                     \
                                     obs_data_type *obs_data,                  \
                                     enkf_fs_type *fs, int report_step) {      \
        prefix##_get_observations((prefix##_type *)void_arg, obs_data, fs,     \
                                  report_step);                                \
    }

#define VOID_GET_OBS_HEADER(prefix)                                            \
    void prefix##_get_observations__(const void *, obs_data_type *,            \
                                     enkf_fs_type *, int)

#define VOID_MEASURE(obs_prefix, state_prefix)                                 \
    void obs_prefix##_measure__(const void *void_obs, const void *void_state,  \
                                node_id_type node_id,                          \
                                meas_data_type *meas_data) {                   \
        const obs_prefix##_type *obs = obs_prefix##_safe_cast_const(void_obs); \
        const state_prefix##_type *state =                                     \
            state_prefix##_safe_cast_const(void_state);                        \
        obs_prefix##_measure(obs, state, node_id, meas_data);                  \
    }

#define VOID_MEASURE_UNSAFE(obs_prefix, state_prefix)                          \
    void obs_prefix##_measure__(const void *void_obs, const void *state,       \
                                node_id_type node_id,                          \
                                meas_data_type *meas_data) {                   \
        const obs_prefix##_type *obs = obs_prefix##_safe_cast_const(void_obs); \
        obs_prefix##_measure(obs, state, node_id, meas_data);                  \
    }

#define VOID_MEASURE_HEADER(obs_prefix)                                        \
    void obs_prefix##_measure__(const void *, const void *, node_id_type,      \
                                meas_data_type *)

#define VOID_UPDATE_STD_SCALE(prefix)                                          \
    void prefix##_update_std_scale__(void *void_obs, double std_multiplier,    \
                                     const ActiveList *active_list) {          \
        prefix##_type *obs = prefix##_safe_cast(void_obs);                     \
        prefix##_update_std_scale(obs, std_multiplier, active_list);           \
    }

#define VOID_UPDATE_STD_SCALE_HEADER(prefix)                                   \
    void prefix##_update_std_scale__(void *void_obs, double std_multiplier,    \
                                     const ActiveList *active_list);

#define VOID_CHI2(obs_prefix, state_prefix)                                    \
    double obs_prefix##_chi2__(const void *void_obs, const void *void_state,   \
                               node_id_type node_id) {                         \
        const obs_prefix##_type *obs = obs_prefix##_safe_cast_const(void_obs); \
        const state_prefix##_type *state =                                     \
            (const state_prefix##_type *)void_state;                           \
        return obs_prefix##_chi2(obs, state, node_id);                         \
    }

#define VOID_CHI2_HEADER(obs_prefix)                                           \
    double obs_prefix##_chi2__(const void *, const void *, node_id_type);

#define VOID_TRUNCATE(prefix)                                                  \
    void prefix##_truncate__(void *void_arg) {                                 \
        prefix##_truncate((prefix##_type *)void_arg);                          \
    }
#define VOID_TRUNCATE_HEADER(prefix) void prefix##_truncate__(void *)

#define VOID_CLEAR(prefix)                                                     \
    void prefix##_clear__(void *void_arg) {                                    \
        prefix##_clear(prefix##_safe_cast(void_arg));                          \
    }
#define VOID_CLEAR_HEADER(prefix) void prefix##_clear__(void *)
#endif

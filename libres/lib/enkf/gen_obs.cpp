/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'gen_obs.c' is part of ERT - Ensemble based Reservoir Tool.

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
   See the overview documentation of the observation system in
   enkf_obs.c
*/
#include <stdlib.h>

#include <ert/util/string_util.h>
#include <ert/util/util.h>

#include <ert/enkf/enkf_fs.hpp>
#include <ert/enkf/enkf_macros.hpp>
#include <ert/enkf/enkf_util.hpp>
#include <ert/enkf/gen_common.hpp>
#include <ert/enkf/gen_data.hpp>
#include <ert/enkf/gen_obs.hpp>

#include "ert/python.hpp"

#define GEN_OBS_TYPE_ID 77619

/**
   This file implemenets a structure for general observations. A
   general observation is just a vector of numbers - where EnKF has no
   understanding whatsover of the type of these data. The actual data
   is supposed to be found in a file.

   Currently it can only observe gen_data instances - but that should
   be generalized.

  The std_scaling field of the xxx_obs structure can be used to scale
  the standard deviation used for the observations, either to support
  workflows with multiple data assimilation or to reduce the effect of
  observation correlations.

  When querying for the observation standard deviation using
  gen_obs_iget_std() the user input value of standard deviation will
  be returned, whereas when the function gen_obs_measure() is used the
  std_scaling will be incorporated in the result.
*/
struct gen_obs_struct {
    UTIL_TYPE_ID_DECLARATION;
    /** This is the total size of the observation vector. */
    int obs_size;
    /** The indexes which are observed in the corresponding gen_data instance -
     * of length obs_size. */
    int *data_index_list;
    /** Flag which indiactes whether all data in the gen_data instance should
     * be observed - in that case we must do a size comparizon-check at use time.*/
    bool observe_all_data;

    /** The observed data. */
    double *obs_data;
    /** The observed standard deviation. */
    double *obs_std;
    /** Scaling factor for the standard deviation */
    double *std_scaling;

    /** The key this observation is held by - in the enkf_obs structur (only
     * for debug messages). */
    char *obs_key;
    /** The format, i.e. ASCII, binary_double or binary_float, of the
     * observation file. */
    gen_data_file_format_type obs_format;
    gen_data_config_type *data_config;
};

static UTIL_SAFE_CAST_FUNCTION_CONST(
    gen_obs, GEN_OBS_TYPE_ID) static UTIL_SAFE_CAST_FUNCTION(gen_obs,
                                                             GEN_OBS_TYPE_ID)

    void gen_obs_free(gen_obs_type *gen_obs) {
    free(gen_obs->obs_data);
    free(gen_obs->obs_std);
    free(gen_obs->data_index_list);
    free(gen_obs->obs_key);
    free(gen_obs->std_scaling);

    free(gen_obs);
}

static double IGET_SCALED_STD(const gen_obs_type *gen_obs, int index) {
    return gen_obs->obs_std[index] * gen_obs->std_scaling[index];
}

/**
   This function loads the actual observations from disk, and
   initializes the obs_data and obs_std pointers with the
   observations. It also sets the obs_size field of the gen_obs
   instance.

   The file with observations should be a long vector of 2N elements,
   where the first N elements are data values, and the last N values
   are the corresponding standard deviations.

   The file is loaded with the gen_common_fload_alloc() function, and
   can be in formatted ASCII or binary_float / binary_double. Observe
   that there is *NO* header information in this file.
*/
static void gen_obs_set_data(gen_obs_type *gen_obs, int buffer_size,
                             const double *buffer) {
    gen_obs->obs_size = buffer_size / 2;
    gen_obs->obs_data = (double *)util_realloc(
        gen_obs->obs_data, gen_obs->obs_size * sizeof *gen_obs->obs_data);
    gen_obs->obs_std = (double *)util_realloc(
        gen_obs->obs_std, gen_obs->obs_size * sizeof *gen_obs->obs_std);
    gen_obs->std_scaling = (double *)util_realloc(
        gen_obs->std_scaling, gen_obs->obs_size * sizeof *gen_obs->std_scaling);
    gen_obs->data_index_list = (int *)util_realloc(
        gen_obs->data_index_list,
        gen_obs->obs_size * sizeof *gen_obs->data_index_list);
    {
        int iobs;
        double *double_buffer = (double *)buffer;
        for (iobs = 0; iobs < gen_obs->obs_size; iobs++) {
            gen_obs->obs_data[iobs] = double_buffer[2 * iobs];
            gen_obs->obs_std[iobs] = double_buffer[2 * iobs + 1];
            gen_obs->std_scaling[iobs] = 1.0;
            gen_obs->data_index_list[iobs] = iobs;
        }
    }
}

void gen_obs_load_observation(gen_obs_type *gen_obs, const char *obs_file) {
    ecl_type_enum load_type;
    void *buffer;
    int buffer_size = 0;
    buffer = gen_common_fload_alloc(obs_file, gen_obs->obs_format, ECL_DOUBLE,
                                    &load_type, &buffer_size);

    /* Ensure that the data is of type double. */
    if (load_type == ECL_FLOAT_TYPE) {
        double *double_data =
            (double *)util_calloc(gen_obs->obs_size, sizeof *double_data);
        util_float_to_double(double_data, (const float *)buffer, buffer_size);
        free(buffer);
        buffer = double_data;
    }

    gen_obs_set_data(gen_obs, buffer_size, (double *)buffer);
    free(buffer);
}

void gen_obs_set_scalar(gen_obs_type *gen_obs, double scalar_value,
                        double scalar_std) {
    double buffer[2] = {scalar_value, scalar_std};
    gen_obs_set_data(gen_obs, 2, buffer);
}

void gen_obs_attach_data_index(gen_obs_type *obs,
                               const int_vector_type *data_index) {
    free(obs->data_index_list);
    obs->data_index_list = int_vector_alloc_data_copy(data_index);
    obs->observe_all_data = false;
}

void gen_obs_load_data_index(gen_obs_type *obs, const char *data_index_file) {
    /* Parsing an a file with integers. */
    free(obs->data_index_list);
    obs->data_index_list = (int *)gen_common_fscanf_alloc(
        data_index_file, ECL_INT, &obs->obs_size);
    obs->observe_all_data = false;
}

void gen_obs_parse_data_index(gen_obs_type *obs,
                              const char *data_index_string) {
    /* Parsing a string of the type "1,3,5,9-100,200,202,300-1000" */
    int_vector_type *index_list =
        string_util_alloc_active_list(data_index_string);
    int_vector_shrink(index_list);
    gen_obs_attach_data_index(obs, index_list);
    int_vector_free(index_list);
}

gen_obs_type *gen_obs_alloc__(const gen_data_config_type *data_config,
                              const char *obs_key) {
    gen_obs_type *obs = (gen_obs_type *)util_malloc(sizeof *obs);
    UTIL_TYPE_ID_INIT(obs, GEN_OBS_TYPE_ID);
    obs->obs_data = NULL;
    obs->obs_std = NULL;
    obs->std_scaling = NULL;
    obs->data_index_list = NULL;
    obs->obs_format = ASCII; /* Hardcoded for now. */
    obs->obs_key = util_alloc_string_copy(obs_key);
    obs->data_config =
        (gen_data_config_type *)data_config; // casting away the const ...
    obs->observe_all_data = true;
    return obs;
}

/**
   data_index_file is the name of a file with indices which should be
   observed, data_inde_string is the same, in the form of a
   "1,2,3,4-10, 17,19,22-100" string. Only one of these items can be
   != NULL. If both are NULL it is assumed that all the indices of the
   gen_data instance should be observed.
*/
gen_obs_type *gen_obs_alloc(const gen_data_config_type *data_config,
                            const char *obs_key, const char *obs_file,
                            double scalar_value, double scalar_error,
                            const char *data_index_file,
                            const char *data_index_string) {
    gen_obs_type *obs = gen_obs_alloc__(data_config, obs_key);
    if (obs_file)
        gen_obs_load_observation(
            obs,
            obs_file); /* The observation data is loaded - and internalized at boot time - even though it might not be needed for a long time. */
    else
        gen_obs_set_scalar(obs, scalar_value, scalar_error);

    if (data_index_file)
        gen_obs_load_data_index(obs, data_index_file);
    else if (data_index_string)
        gen_obs_parse_data_index(obs, data_index_string);

    return obs;
}

static void gen_obs_assert_data_size(const gen_obs_type *gen_obs,
                                     const gen_data_type *gen_data) {
    if (gen_obs->observe_all_data) {
        int data_size = gen_data_get_size(gen_data);
        if (gen_obs->obs_size != data_size)
            util_abort(
                "%s: size mismatch: Observation: %s:%d      Data: %s:%d \n",
                __func__, gen_obs->obs_key, gen_obs->obs_size,
                gen_data_get_key(gen_data), data_size);
    }
    /*
    Else the user has explicitly entered indices to observe in the
    gen_data instances, and we just have to trust them (however the
    gen_data_iget() does a range check.
  */
}

double gen_obs_chi2(const gen_obs_type *gen_obs, const gen_data_type *gen_data,
                    node_id_type node_id) {
    gen_obs_assert_data_size(gen_obs, gen_data);
    {
        const bool_vector_type *forward_model_active =
            gen_data_config_get_active_mask(gen_obs->data_config);
        double sum_chi2 = 0;
        for (int iobs = 0; iobs < gen_obs->obs_size; iobs++) {
            int data_index = gen_obs->data_index_list[iobs];
            if (forward_model_active &&
                (bool_vector_iget(forward_model_active, data_index) == false))
                continue; /* Forward model has deactivated this index - just continue. */
            {
                double d = gen_data_iget_double(gen_data, data_index);
                double x =
                    (d - gen_obs->obs_data[iobs]) / gen_obs->obs_std[iobs];
                sum_chi2 += x * x;
            }
        }
        return sum_chi2;
    }
}

void gen_obs_measure(const gen_obs_type *gen_obs, const gen_data_type *gen_data,
                     node_id_type node_id, meas_data_type *meas_data) {
    gen_obs_assert_data_size(gen_obs, gen_data);
    {
        meas_block_type *meas_block =
            meas_data_add_block(meas_data, gen_obs->obs_key,
                                node_id.report_step, gen_obs->obs_size);
        const bool_vector_type *forward_model_active =
            gen_data_config_get_active_mask(gen_obs->data_config);

        for (int iobs = 0; iobs < gen_obs->obs_size; iobs++) {
            int data_index = gen_obs->data_index_list[iobs];

            if (forward_model_active != NULL) {
                if (!bool_vector_iget(forward_model_active, data_index))
                    continue; /* Forward model has deactivated this index - just continue. */
            }

            meas_block_iset(meas_block, node_id.iens, iobs,
                            gen_data_iget_double(gen_data, data_index));
        }
    }
}

C_USED void gen_obs_get_observations(gen_obs_type *gen_obs,
                                     obs_data_type *obs_data, enkf_fs_type *fs,
                                     int report_step) {
    const bool_vector_type *forward_model_active = NULL;
    if (gen_data_config_has_active_mask(gen_obs->data_config, fs,
                                        report_step)) {
        gen_data_config_load_active(gen_obs->data_config, fs, report_step,
                                    true);
        forward_model_active =
            gen_data_config_get_active_mask(gen_obs->data_config);
    }

    {
        obs_block_type *obs_block =
            obs_data_add_block(obs_data, gen_obs->obs_key, gen_obs->obs_size);

        for (int iobs = 0; iobs < gen_obs->obs_size; iobs++)
            obs_block_iset(obs_block, iobs, gen_obs->obs_data[iobs],
                           IGET_SCALED_STD(gen_obs, iobs));

        /* Setting some of the elements as missing, i.e. deactivated by the forward model. */
        if (forward_model_active != NULL) {
            for (int iobs = 0; iobs < gen_obs->obs_size; iobs++) {
                int data_index = gen_obs->data_index_list[iobs];
                if (!bool_vector_iget(forward_model_active, data_index))
                    obs_block_iset_missing(obs_block, iobs);
            }
        }
    }
}

/*
   In general the gen_obs observation vector can be smaller than the
   gen_data field it is observing, i.e. we can have a situation like
   this:

           Data               Obs
           ----               ---

          [ 6.0 ] ----\
          [ 2.0 ]      \---> [ 6.3 ]
          [ 3.0 ] ---------> [ 2.8 ]
          [ 2.0 ]      /---> [ 4.3 ]
          [ 4.5 ] ----/

   The situation here is as follows:

   1. We have a gen data vector with five elements.

   2. We have an observation vector of three elements, which observes
      three of the elements in the gen_data vector, in this particular
      case the data_index_list of the observation equals: [0 , 2 , 4].

   Now when we want to look at the match of observation quality of the
   last element in the observation vector it would be natural to use
   the user_get key: "obs_key:2" - however this is an observation of
   data element number 4, i.e. as seen from data context (when adding
   observations to an ensemble plot) the natural indexing would be:
   "data_key:4".


   The function gen_obs_user_get_with_data_index() will do the
   translation from data based indexing to observation based indexing, i.e.

      gen_obs_user_get_with_data_index("4")

   will do an inverse lookup of the '4' and further call

      gen_obs_user_get("2")

*/

void gen_obs_user_get(const gen_obs_type *gen_obs, const char *index_key,
                      double *value, double *std, bool *valid) {
    int index;
    *valid = false;

    if (util_sscanf_int(index_key, &index)) {
        if ((index >= 0) && (index < gen_obs->obs_size)) {
            *valid = true;
            *value = gen_obs->obs_data[index];
            *std = gen_obs->obs_std[index];
        }
    }
}

void gen_obs_user_get_with_data_index(const gen_obs_type *gen_obs,
                                      const char *index_key, double *value,
                                      double *std, bool *valid) {
    if (gen_obs->observe_all_data)
        /* The observation and data vectors are equally long - no reverse lookup necessary. */
        gen_obs_user_get(gen_obs, index_key, value, std, valid);
    else {
        *valid = false;
        int data_index;
        if (util_sscanf_int(index_key, &data_index)) {
            int obs_index = 0;
            do {
                if (gen_obs->data_index_list[obs_index] == data_index)
                    /* Found it - will use the 'obs_index' value. */
                    break;

                obs_index++;
            } while (obs_index < gen_obs->obs_size);
            if (obs_index <
                gen_obs->obs_size) { /* The reverse lookup succeeded. */
                *valid = true;
                *value = gen_obs->obs_data[obs_index];
                *std = gen_obs->obs_std[obs_index];
            }
        }
    }
}

void gen_obs_update_std_scale(gen_obs_type *gen_obs, double std_multiplier,
                              const ActiveList *active_list) {
    if (active_list->getMode() == ALL_ACTIVE) {
        for (int i = 0; i < gen_obs->obs_size; i++)
            gen_obs->std_scaling[i] = std_multiplier;
    } else {
        const int *active_index = active_list->active_list_get_active();
        int size = active_list->active_size(gen_obs->obs_size);
        for (int i = 0; i < size; i++) {
            int obs_index = active_index[i];
            if (obs_index >= gen_obs->obs_size) {
                util_abort("[Gen_Obs] Index out of bounds %d [0, %d]",
                           obs_index, gen_obs->obs_size - 1);
            }
            gen_obs->std_scaling[obs_index] = std_multiplier;
        }
    }
}

int gen_obs_get_size(const gen_obs_type *gen_obs) { return gen_obs->obs_size; }

double gen_obs_iget_std(const gen_obs_type *gen_obs, int index) {
    return gen_obs->obs_std[index];
}

double gen_obs_iget_std_scaling(const gen_obs_type *gen_obs, int index) {
    return gen_obs->std_scaling[index];
}

double gen_obs_iget_value(const gen_obs_type *gen_obs, int index) {
    return gen_obs->obs_data[index];
}

void gen_obs_load_values(const gen_obs_type *gen_obs, int size, double *data) {
    for (int i = 0; i < size; i++) {
        data[i] = gen_obs->obs_data[i];
    }
}

void gen_obs_load_std(const gen_obs_type *gen_obs, int size, double *data) {
    for (int i = 0; i < size; i++) {
        data[i] = gen_obs->obs_std[i];
    }
}

int gen_obs_get_obs_index(const gen_obs_type *gen_obs, int index) {
    if (index < 0 || index >= gen_obs->obs_size) {
        util_abort("[Gen_Obs] Index out of bounds %d [0, %d]", index,
                   gen_obs->obs_size - 1);
    }

    if (gen_obs->observe_all_data) {
        return index;
    } else {
        return gen_obs->data_index_list[index];
    }
}

UTIL_IS_INSTANCE_FUNCTION(gen_obs, GEN_OBS_TYPE_ID)
VOID_FREE(gen_obs)
VOID_GET_OBS(gen_obs)
VOID_MEASURE(gen_obs, gen_data)
VOID_USER_GET_OBS(gen_obs)
VOID_CHI2(gen_obs, gen_data)
VOID_UPDATE_STD_SCALE(gen_obs)

class ActiveList;
namespace {
void update_std_scaling(py::handle obj, double scaling,
                        const ActiveList &active_list) {
    auto *self = ert::from_cwrap<gen_obs_type>(obj);
    gen_obs_update_std_scale(self, scaling, &active_list);
}
} // namespace

RES_LIB_SUBMODULE("local.gen_obs", m) {
    using namespace py::literals;

    m.def("update_std_scaling", &update_std_scaling, "self"_a, "scaling"_a,
          "active_list"_a);
}

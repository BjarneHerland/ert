/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'gen_kw.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <Eigen/Dense>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ert/res_util/subst_list.hpp>
#include <ert/util/buffer.h>
#include <ert/util/util.h>

#include <ert/enkf/enkf_macros.hpp>
#include <ert/enkf/enkf_types.hpp>
#include <ert/enkf/enkf_util.hpp>
#include <ert/enkf/gen_kw.hpp>
#include <ert/enkf/gen_kw_common.hpp>
#include <ert/enkf/gen_kw_config.hpp>

GET_DATA_SIZE_HEADER(gen_kw);

struct gen_kw_struct {
    int __type_id;
    const gen_kw_config_type *config;
    double *data;
    subst_list_type *subst_list;
};

void gen_kw_free(gen_kw_type *gen_kw) {
    free(gen_kw->data);
    subst_list_free(gen_kw->subst_list);
    free(gen_kw);
}

extern "C" PY_USED gen_kw_type *gen_kw_alloc(const gen_kw_config_type *config) {
    gen_kw_type *gen_kw = (gen_kw_type *)util_malloc(sizeof *gen_kw);
    gen_kw->__type_id = GEN_KW;
    gen_kw->config = config;
    gen_kw->subst_list = subst_list_alloc(NULL);
    gen_kw->data = (double *)util_calloc(gen_kw_config_get_data_size(config),
                                         sizeof *gen_kw->data);
    return gen_kw;
}

C_USED void gen_kw_clear(gen_kw_type *gen_kw) {
    int i;
    for (i = 0; i < gen_kw_config_get_data_size(gen_kw->config); i++)
        gen_kw->data[i] = 0.0;
}

void gen_kw_copy(const gen_kw_type *src, gen_kw_type *target) {
    if (src->config == target->config) {
        int buffer_size =
            gen_kw_config_get_data_size(src->config) * sizeof src->data;
        memcpy(target->data, src->data, buffer_size);
    } else
        util_abort("%s: two elements do not share config object \n", __func__);
}

int gen_kw_data_size(const gen_kw_type *gen_kw) {
    return gen_kw_config_get_data_size(gen_kw->config);
}

double gen_kw_data_iget(const gen_kw_type *gen_kw, int index,
                        bool do_transform) {
    double value;
    int size = gen_kw_config_get_data_size(gen_kw->config);
    if ((index < 0) || (index >= size))
        util_abort("%s: index:%d invalid. Valid interval: [0,%d>.\n", __func__,
                   index, size);

    if (do_transform) {
        value =
            gen_kw_config_transform(gen_kw->config, index, gen_kw->data[index]);
    } else {
        value = gen_kw->data[index];
    }

    return value;
}

void gen_kw_data_set_vector(gen_kw_type *gen_kw,
                            const double_vector_type *values) {
    int size = gen_kw_config_get_data_size(gen_kw->config);
    if (size == double_vector_size(values)) {
        for (int index = 0; index < size; index++)
            gen_kw->data[index] = double_vector_iget(values, index);
    } else
        util_abort("%s: Invalid size for vector:%d  gen_Kw:%d \n", __func__,
                   double_vector_size(values), size);
}

void gen_kw_data_iset(gen_kw_type *gen_kw, int index, double value) {
    int size = gen_kw_config_get_data_size(gen_kw->config);
    if ((index < 0) || (index >= size))
        util_abort("%s: index:%d invalid. Valid interval: [0,%d>.\n", __func__,
                   index, size);

    gen_kw->data[index] = value;
}

double gen_kw_data_get(gen_kw_type *gen_kw, const char *subkey,
                       bool do_transform) {
    int index = gen_kw_config_get_index(gen_kw->config, subkey);
    return gen_kw_data_iget(gen_kw, index, do_transform);
}

void gen_kw_data_set(gen_kw_type *gen_kw, const char *subkey, double value) {
    int index = gen_kw_config_get_index(gen_kw->config, subkey);
    return gen_kw_data_iset(gen_kw, index, value);
}

bool gen_kw_data_has_key(gen_kw_type *gen_kw, const char *subkey) {
    int index = gen_kw_config_get_index(gen_kw->config, subkey);
    bool has_key =
        ((0 <= index) && (gen_kw_data_size(gen_kw) > index)) ? true : false;
    return has_key;
}

C_USED bool gen_kw_write_to_buffer(const gen_kw_type *gen_kw,
                                   buffer_type *buffer, int report_step) {
    const int data_size = gen_kw_config_get_data_size(gen_kw->config);
    buffer_fwrite_int(buffer, GEN_KW);
    buffer_fwrite(buffer, gen_kw->data, sizeof *gen_kw->data, data_size);
    return true;
}

/*
   As of 17/03/09 (svn 1811) MULTFLT has been depreceated, and GEN_KW
   has been inserted as a 'drop-in-replacement'. This implies that
   existing storage labeled with implemantation type 'MULTFLT' should
   be silently 'upgraded' to 'GEN_KW'.
*/

#define MULTFLT 102
void gen_kw_read_from_buffer(gen_kw_type *gen_kw, buffer_type *buffer,
                             enkf_fs_type *fs, int report_step) {
    const int data_size = gen_kw_config_get_data_size(gen_kw->config);
    ert_impl_type file_type;
    file_type = (ert_impl_type)buffer_fread_int(buffer);
    if ((file_type == GEN_KW) || (file_type == MULTFLT))
        buffer_fread(buffer, gen_kw->data, sizeof *gen_kw->data, data_size);
}
#undef MULTFLT

C_USED static bool gen_kw_fload(gen_kw_type *, const char *);

bool gen_kw_initialize(gen_kw_type *gen_kw, int iens, const char *init_file,
                       rng_type *rng) {
    if (!init_file && !rng)
        util_abort("%s internal error: both init_file and rng are NULL",
                   __func__);

    bool ret = false;

    if (init_file)
        ret = gen_kw_fload(gen_kw, init_file);
    else {
        const double mean =
            0.0; /* Mean and std are hardcoded - the variability should be in the transformation. */
        const double std = 1.0;
        const int data_size = gen_kw_config_get_data_size(gen_kw->config);
        int i;

        for (i = 0; i < data_size; i++)
            gen_kw->data[i] = enkf_util_rand_normal(mean, std, rng);

        ret = true;
    }
    return ret;
}

void gen_kw_serialize(const gen_kw_type *gen_kw, node_id_type node_id,
                      const ActiveList *active_list, Eigen::MatrixXd &A,
                      int row_offset, int column) {
    const int data_size = gen_kw_config_get_data_size(gen_kw->config);
    enkf_matrix_serialize(gen_kw->data, data_size, ECL_DOUBLE, active_list, A,
                          row_offset, column);
}

void gen_kw_deserialize(gen_kw_type *gen_kw, node_id_type node_id,
                        const ActiveList *active_list, const Eigen::MatrixXd &A,
                        int row_offset, int column) {
    const int data_size = gen_kw_config_get_data_size(gen_kw->config);
    enkf_matrix_deserialize(gen_kw->data, data_size, ECL_DOUBLE, active_list, A,
                            row_offset, column);
}

void gen_kw_filter_file(const gen_kw_type *gen_kw, const char *target_file) {
    const char *template_file = gen_kw_config_get_template_file(gen_kw->config);
    if (template_file != NULL) {
        const int size = gen_kw_config_get_data_size(gen_kw->config);
        int ikw;

        for (ikw = 0; ikw < size; ikw++) {
            const char *key =
                gen_kw_config_get_tagged_name(gen_kw->config, ikw);
            subst_list_append_owned_ref(
                gen_kw->subst_list, key,
                util_alloc_sprintf("%g",
                                   gen_kw_config_transform(gen_kw->config, ikw,
                                                           gen_kw->data[ikw])),
                NULL);
        }

        /*
      If the target_file already exists as a symbolic link the
      symbolic link is removed before creating the target file. The is
      to ensure against existing symlinks pointing to a common file
      outside the realization root.
    */
        if (util_is_link(target_file))
            remove(target_file);

        subst_list_filter_file(gen_kw->subst_list, template_file, target_file);
    } else
        util_abort("%s: internal error - tried to filter gen_kw instance "
                   "without template file.\n",
                   __func__);
}

void gen_kw_export_values(const gen_kw_type *gen_kw,
                          value_export_type *export_value) {
    const int size = gen_kw_config_get_data_size(gen_kw->config);

    for (int ikw = 0; ikw < size; ++ikw) {
        const char *key = gen_kw_config_get_key(gen_kw->config);
        const char *parameter = gen_kw_config_iget_name(gen_kw->config, ikw);

        double value =
            gen_kw_config_transform(gen_kw->config, ikw, gen_kw->data[ikw]);

        value_export_append(export_value, key, parameter, value);

        if (gen_kw_config_should_use_log_scale(gen_kw->config, ikw)) {
            double log_value = log10(value);
            char *log_key = util_alloc_sprintf("LOG10_%s", key);
            value_export_append(export_value, log_key, parameter, log_value);
            free(log_key);
        }
    }
}

void gen_kw_write_export_file(const gen_kw_type *gen_kw, const char *filename) {
    value_export_type *export_value = value_export_alloc("", filename);
    gen_kw_export_values(gen_kw, export_value);
    value_export_txt__(export_value, filename);
    value_export_free(export_value);
}

void gen_kw_ecl_write(const gen_kw_type *gen_kw, const char *run_path,
                      const char *base_file, value_export_type *export_value) {
    char *target_file;
    if (run_path)
        target_file = util_alloc_filename(run_path, base_file, NULL);
    else
        target_file = util_alloc_string_copy(base_file);

    gen_kw_filter_file(gen_kw, target_file);
    free(target_file);

    if (export_value)
        gen_kw_export_values(gen_kw, export_value);
}

const char *gen_kw_get_name(const gen_kw_type *gen_kw, int kw_nr) {
    return gen_kw_config_iget_name(gen_kw->config, kw_nr);
}

/**
   This function will load values for gen_kw instance from file. The
   file should be formatted as either:

   -------
   Value1
   Value2
   Value3
   ....
   ValueN
   -------

   Or

   ------------
   Key3  Value3
   Key5  Value5
   Key1  Value1
   .....
   ------------

   I.e. you can either just dump in all the numbers in one long
   vector, or you can interlace numbers and keys. In the latter case
   the ordering is arbitrary.

   Observe the following:

    1. All values must be specified.
    2. The values are in the N(0,1) domain, i.e. the untransformed variables.

*/
static bool gen_kw_fload(gen_kw_type *gen_kw, const char *filename) {
    FILE *stream = util_fopen__(filename, "r");
    if (stream) {
        const int size = gen_kw_config_get_data_size(gen_kw->config);
        bool readOK = true;

        /* First try reading all the data as one long vector. */
        {
            int index = 0;
            while ((index < size) && readOK) {
                double value;
                if (fscanf(stream, "%lg", &value) == 1)
                    gen_kw->data[index] = value;
                else
                    readOK = false;
                index++;
            }
        }

        /*
       OK - rewind and try again with interlaced key + value
       pairs. Observe that we still require that ALL the elements in the
       gen_kw instance are set, i.e. it is not allowed to read only some
       of the keywords; but the ordering is not relevant.

       The code will be fooled (and give undefined erronous results) if
       the same key appears several times. Be polite!
    */

        if (!readOK) {
            int counter = 0;
            readOK = true;
            fseek(stream, 0, SEEK_SET);

            while ((counter < size) && readOK) {
                char key[128];
                double value;
                int fscanf_return = fscanf(stream, "%s %lg", key, &value);

                if (fscanf_return == 2) {
                    int index = gen_kw_config_get_index(gen_kw->config, key);
                    if (index >= 0)
                        gen_kw->data[index] = value;
                    else
                        util_abort(
                            "%s: key:%s not recognized as part of GEN_KW "
                            "instance - error when reading file:%s \n",
                            __func__, key, filename);
                    counter++;
                } else {
                    util_abort("%s: failed to read (key,value) pair at line:%d "
                               "in file:%s \n",
                               __func__, util_get_current_linenr(stream),
                               filename);
                    readOK = false;
                }
            }
        }

        if (!readOK)
            util_abort("%s: failed loading from file:%s \n", __func__,
                       filename);

        fclose(stream);
        return true;
    } else
        return false;
}

/**
   Will return 0.0 on invalid input, and set valid -> false. It is the
   responsibility of the calling scope to check valid.
*/
C_USED bool gen_kw_user_get(const gen_kw_type *gen_kw, const char *key,
                            int report_step, double *value) {
    int index = gen_kw_config_get_index(gen_kw->config, key);

    if (index >= 0) {
        *value =
            gen_kw_config_transform(gen_kw->config, index, gen_kw->data[index]);
        return true;
    } else {
        *value = 0.0;
        fprintf(stderr,
                "** Warning:could not lookup key:%s in gen_kw instance \n",
                key);
        return false;
    }
}

UTIL_SAFE_CAST_FUNCTION(gen_kw, GEN_KW);
UTIL_SAFE_CAST_FUNCTION_CONST(gen_kw, GEN_KW);
VOID_ALLOC(gen_kw);
VOID_INITIALIZE(gen_kw);
VOID_COPY(gen_kw)
VOID_FREE(gen_kw)
VOID_ECL_WRITE(gen_kw)
VOID_USER_GET(gen_kw)
VOID_WRITE_TO_BUFFER(gen_kw)
VOID_READ_FROM_BUFFER(gen_kw)
VOID_SERIALIZE(gen_kw)
VOID_DESERIALIZE(gen_kw)
VOID_CLEAR(gen_kw)
VOID_FLOAD(gen_kw)

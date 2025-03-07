/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'gen_data.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <filesystem>

#include <Eigen/Dense>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ert/util/util.h>

#include <ert/ecl/ecl_sum.h>

#include <ert/logging.hpp>

#include <ert/enkf/enkf_macros.hpp>
#include <ert/enkf/enkf_serialize.hpp>
#include <ert/enkf/enkf_util.hpp>
#include <ert/enkf/gen_common.hpp>
#include <ert/enkf/gen_data.hpp>
#include <ert/enkf/gen_data_config.hpp>

namespace fs = std::filesystem;
static auto logger = ert::get_logger("enkf");

/**
   A general data type which can be used to update
   arbitrary data which the EnKF system has *ABSOLUTELY NO IDEA* of
   how is organised; how it should be used in the forward model and so
   on. Similarly to the field objects, the gen_data objects can be
   treated both as parameters and as dynamic data.

   Whether the forward_load function should be called (i.e. it is dynamic
   data) is determined at the enkf_node level, and no busissiness of
   the gen_data implementation.
*/
struct gen_data_struct {
    int __type_id;
    /** Thin config object - mainly contains filename for remote load */
    gen_data_config_type *config;
    /** Actual storage - will be casted to double or float on use. */
    char *data;
    /** Need this to look up the correct size in the config object. */
    int current_report_step;
    /** Mask of active/not active - loaded from a "_active" file created by the
     * forward model. Not used when used as parameter*/
    bool_vector_type *active_mask;
};

void gen_data_assert_size(gen_data_type *gen_data, int size, int report_step) {
    gen_data_config_assert_size(gen_data->config, size, report_step);
    gen_data->current_report_step = report_step;
}

gen_data_config_type *gen_data_get_config(const gen_data_type *gen_data) {
    return gen_data->config;
}

int gen_data_get_size(const gen_data_type *gen_data) {
    return gen_data_config_get_data_size(gen_data->config,
                                         gen_data->current_report_step);
}

/**
   It is a bug to call this before some function has set the size.
*/
void gen_data_realloc_data(gen_data_type *gen_data) {
    int byte_size = gen_data_config_get_byte_size(
        gen_data->config, gen_data->current_report_step);
    gen_data->data = (char *)util_realloc(gen_data->data, byte_size);
}

gen_data_type *gen_data_alloc(const gen_data_config_type *config) {
    gen_data_type *gen_data = (gen_data_type *)util_malloc(sizeof *gen_data);
    gen_data->config = (gen_data_config_type *)config;
    gen_data->data = NULL;
    gen_data->__type_id = GEN_DATA;
    gen_data->active_mask = bool_vector_alloc(0, true);
    gen_data->current_report_step = -1; /* God - if you ever read this .... */
    return gen_data;
}

void gen_data_copy(const gen_data_type *src, gen_data_type *target) {
    if (src->config == target->config) {
        target->current_report_step = src->current_report_step;

        if (src->data != NULL) {
            int byte_size = gen_data_config_get_byte_size(
                src->config, src->current_report_step);
            target->data =
                (char *)util_realloc_copy(target->data, src->data, byte_size);
        }
    } else
        util_abort("%s: do not share config object \n", __func__);
}

void gen_data_free(gen_data_type *gen_data) {
    free(gen_data->data);
    bool_vector_free(gen_data->active_mask);
    free(gen_data);
}

/**
   Observe that this function writes parameter size to disk, that is
   special. The reason is that the config object does not know the
   size (on allocation).

   The function currently writes an empty file (with only a report
   step and a size == 0) in the case where it does not have data. This
   is controlled by the value of the variable write_zero_size; if this
   is changed to false some semantics in the load code must be
   changed.
*/
C_USED bool gen_data_write_to_buffer(const gen_data_type *gen_data,
                                     buffer_type *buffer, int report_step) {
    const bool write_zero_size =
        true; /* true:ALWAYS write a file   false:only write files with size > 0. */
    {
        bool write = write_zero_size;
        int size = gen_data_config_get_data_size(gen_data->config, report_step);
        if (size > 0)
            write = true;

        if (write) {
            int byte_size =
                gen_data_config_get_byte_size(gen_data->config, report_step);
            buffer_fwrite_int(buffer, GEN_DATA);
            buffer_fwrite_int(buffer, size);
            buffer_fwrite_int(
                buffer,
                report_step); /* Why the heck do I need to store this ????  It was a mistake ...*/

            buffer_fwrite_compressed(buffer, gen_data->data, byte_size);
            return true;
        } else
            return false; /* When false is returned - the (empty) file will be removed */
    }
}

C_USED void gen_data_read_from_buffer(gen_data_type *gen_data,
                                      buffer_type *buffer, enkf_fs_type *fs,
                                      int report_step) {
    int size;
    enkf_util_assert_buffer_type(buffer, GEN_DATA);
    size = buffer_fread_int(buffer);
    buffer_fskip_int(
        buffer); /* Skipping report_step from the buffer - was a mistake to store it - I think ... */
    {
        size_t byte_size =
            size *
            ecl_type_get_sizeof_ctype(
                gen_data_config_get_internal_data_type(gen_data->config));
        size_t compressed_size = buffer_get_remaining_size(buffer);
        gen_data->data = (char *)util_realloc(gen_data->data, byte_size);
        buffer_fread_compressed(buffer, compressed_size, gen_data->data,
                                byte_size);
    }
    gen_data_assert_size(gen_data, size, report_step);

    if (gen_data_config_is_dynamic(gen_data->config)) {
        gen_data_config_load_active(gen_data->config, fs, report_step, false);
    }
}

void gen_data_serialize(const gen_data_type *gen_data, node_id_type node_id,
                        const ActiveList *active_list, Eigen::MatrixXd &A,
                        int row_offset, int column) {
    const gen_data_config_type *config = gen_data->config;
    const int data_size = gen_data_config_get_data_size(
        gen_data->config, gen_data->current_report_step);
    ecl_data_type data_type = gen_data_config_get_internal_data_type(config);

    enkf_matrix_serialize(gen_data->data, data_size, data_type, active_list, A,
                          row_offset, column);
}

void gen_data_deserialize(gen_data_type *gen_data, node_id_type node_id,
                          const ActiveList *active_list,
                          const Eigen::MatrixXd &A, int row_offset,
                          int column) {
    {
        const gen_data_config_type *config = gen_data->config;
        const int data_size = gen_data_config_get_data_size(
            gen_data->config, gen_data->current_report_step);
        ecl_data_type data_type =
            gen_data_config_get_internal_data_type(config);

        enkf_matrix_deserialize(gen_data->data, data_size, data_type,
                                active_list, A, row_offset, column);
    }
}

/**
  This function sets the data field of the gen_data instance after the
  data has been loaded from file.
*/
static void gen_data_set_data__(gen_data_type *gen_data, int size,
                                const forward_load_context_type *load_context,
                                ecl_data_type load_data_type,
                                const void *data) {
    gen_data_assert_size(gen_data, size,
                         forward_load_context_get_load_step(load_context));
    if (gen_data_config_is_dynamic(gen_data->config))
        gen_data_config_update_active(gen_data->config, load_context,
                                      gen_data->active_mask);

    gen_data_realloc_data(gen_data);

    if (size > 0) {
        ecl_data_type internal_type =
            gen_data_config_get_internal_data_type(gen_data->config);
        int byte_size = ecl_type_get_sizeof_ctype(internal_type) * size;

        if (ecl_type_is_equal(load_data_type, internal_type))
            memcpy(gen_data->data, data, byte_size);
        else {
            if (ecl_type_is_float(load_data_type))
                util_float_to_double((double *)gen_data->data,
                                     (const float *)data, size);
            else
                util_double_to_float((float *)gen_data->data,
                                     (const double *)data, size);
        }
    }
}

static bool gen_data_fload_active__(gen_data_type *gen_data,
                                    const char *filename, int size) {
    /*
     Look for file @filename_active - if that file is found it is
     interpreted as a an active|inactive mask created by the forward
     model.

     The file is assumed to be an ASCII file with integers, 0
     indicates inactive elements and 1 active elements. The file
     should of course be as long as @filename.

     If the file is not found the gen_data->active_mask is set to
     all-true (i.e. the default true value is invoked).
  */
    bool file_exists = false;
    if (gen_data_config_is_dynamic(gen_data->config)) {
        bool_vector_reset(gen_data->active_mask);
        bool_vector_iset(gen_data->active_mask, size - 1, true);
        {
            char *active_file = util_alloc_sprintf("%s_active", filename);
            if (fs::exists(active_file)) {
                file_exists = true;
                FILE *stream = util_fopen(active_file, "r");
                int active_int;
                for (int index = 0; index < size; index++) {
                    if (fscanf(stream, "%d", &active_int) == 1) {
                        if (active_int == 1)
                            bool_vector_iset(gen_data->active_mask, index,
                                             true);
                        else if (active_int == 0)
                            bool_vector_iset(gen_data->active_mask, index,
                                             false);
                        else
                            util_abort("%s: error when loading active mask "
                                       "from:%s only 0 and 1 allowed \n",
                                       __func__, active_file);
                    } else
                        util_abort("%s: error when loading active mask from:%s "
                                   "- file not long enough.\n",
                                   __func__, active_file);
                }
                fclose(stream);
                logger->info("GEN_DATA({}): active information loaded from:{}.",
                             gen_data_get_key(gen_data), active_file);
            } else
                logger->info("GEN_DATA({}): active information NOT loaded.",
                             gen_data_get_key(gen_data));
            free(active_file);
        }
    }
    return file_exists;
}

/**
   This functions loads data from file. Observe that there is *NO*
   header information in this file - the size is determined by seeing
   how much can be successfully loaded.

   The file is loaded with the gen_common_fload_alloc() function, and
   can be in formatted ASCII or binary_float / binary_double.

   When the read is complete it is checked/verified with the config
   object that this file was as long as the others we have loaded for
   other members; it is perfectly OK for the file to not exist. In
   which case a size of zero is set, for this report step.

   Return value is whether file was found or was empty
  - might have to check this in calling scope.
*/
bool gen_data_fload_with_report_step(
    gen_data_type *gen_data, const char *filename,
    const forward_load_context_type *load_context) {
    bool file_exists = fs::exists(filename);
    void *buffer = NULL;
    if (file_exists) {
        ecl_type_enum load_type;
        ecl_data_type internal_type =
            gen_data_config_get_internal_data_type(gen_data->config);
        gen_data_file_format_type input_format =
            gen_data_config_get_input_format(gen_data->config);
        int size = 0;
        buffer = gen_common_fload_alloc(filename, input_format, internal_type,
                                        &load_type, &size);
        logger->info("GEN_DATA({}): loading from: {}   size:{}",
                     gen_data_get_key(gen_data), filename, size);
        if (size > 0) {
            gen_data_fload_active__(gen_data, filename, size);
        } else {
            bool_vector_reset(gen_data->active_mask);
        }
        gen_data_set_data__(gen_data, size, load_context,
                            ecl_type_create_from_type(load_type), buffer);
        free(buffer);
    } else
        logger->warning("GEN_DATA({}): missing file: {}",
                        gen_data_get_key(gen_data), filename);

    return file_exists;
}

bool gen_data_forward_load(gen_data_type *gen_data, const char *ecl_file,
                           const forward_load_context_type *load_context) {
    return gen_data_fload_with_report_step(gen_data, ecl_file, load_context);
}

/**
   This function initializes the parameter. This is based on loading a
   file. The name of the file is derived from a path_fmt instance
   owned by the config object. Observe that there is *NO* header
   information in this file. We just read floating point numbers until
   we reach EOF.

   When the read is complete it is checked/verified with the config
   object that this file was as long as the files we have loaded for
   other members.

   If gen_data_config_alloc_initfile() returns NULL that means that
   the gen_data instance does not have any init function - that is OK.
*/
C_USED bool gen_data_initialize(gen_data_type *gen_data, int iens,
                                const char *init_file, rng_type *rng) {
    bool ret = false;
    if (init_file) {
        forward_load_context_type *load_context =
            forward_load_context_alloc(NULL, false, NULL);

        forward_load_context_select_step(load_context, 0);
        if (!gen_data_fload_with_report_step(gen_data, init_file, load_context))
            util_abort("%s: could not find file:%s \n", __func__, init_file);
        ret = true;

        forward_load_context_free(load_context);
    }
    return ret;
}

static void gen_data_ecl_write_ASCII(const gen_data_type *gen_data,
                                     const char *file,
                                     gen_data_file_format_type export_format) {
    FILE *stream = util_fopen(file, "w");
    char *template_buffer;
    int template_data_offset, template_buffer_size, template_data_skip;

    if (export_format == ASCII_TEMPLATE) {
        gen_data_config_get_template_data(
            gen_data->config, &template_buffer, &template_data_offset,
            &template_buffer_size, &template_data_skip);
        util_fwrite(template_buffer, 1, template_data_offset, stream, __func__);
    }

    {
        ecl_data_type internal_type =
            gen_data_config_get_internal_data_type(gen_data->config);
        const int size = gen_data_config_get_data_size(
            gen_data->config, gen_data->current_report_step);
        int i;
        if (ecl_type_is_float(internal_type)) {
            float *float_data = (float *)gen_data->data;
            for (i = 0; i < size; i++)
                fprintf(stream, "%g\n", float_data[i]);
        } else if (ecl_type_is_double(internal_type)) {
            double *double_data = (double *)gen_data->data;
            for (i = 0; i < size; i++)
                fprintf(stream, "%lg\n", double_data[i]);
        } else
            util_abort("%s: internal error - wrong type \n", __func__);
    }

    if (export_format == ASCII_TEMPLATE) {
        int new_offset = template_data_offset + template_data_skip;
        util_fwrite(&template_buffer[new_offset], 1,
                    template_buffer_size - new_offset, stream, __func__);
    }
    fclose(stream);
}

static void gen_data_ecl_write_binary(const gen_data_type *gen_data,
                                      const char *file,
                                      ecl_data_type export_type) {
    FILE *stream = util_fopen(file, "w");
    int sizeof_ctype = ecl_type_get_sizeof_ctype(export_type);
    util_fwrite(gen_data->data, sizeof_ctype,
                gen_data_config_get_data_size(gen_data->config,
                                              gen_data->current_report_step),
                stream, __func__);
    fclose(stream);
}

void gen_data_export(const gen_data_type *gen_data, const char *full_path,
                     gen_data_file_format_type export_type) {
    switch (export_type) {
    case (ASCII):
        gen_data_ecl_write_ASCII(gen_data, full_path, export_type);
        break;
    case (ASCII_TEMPLATE):
        gen_data_ecl_write_ASCII(gen_data, full_path, export_type);
        break;
    case (BINARY_DOUBLE):
        gen_data_ecl_write_binary(gen_data, full_path, ECL_DOUBLE);
        break;
    case (BINARY_FLOAT):
        gen_data_ecl_write_binary(gen_data, full_path, ECL_FLOAT);
        break;
    default:
        util_abort("%s: internal error - export type is not set.\n", __func__);
    }
}

/**
    It is the enkf_node layer which knows whether the node actually
    has any data to export. If it is not supposed to write data to the
    forward model, i.e. it is of enkf_type 'dynamic_result' that is
    signaled down here with eclfile == NULL.
*/
void gen_data_ecl_write(const gen_data_type *gen_data, const char *run_path,
                        const char *eclfile, value_export_type *export_value) {
    if (eclfile != NULL) {
        char *full_path = util_alloc_filename(run_path, eclfile, NULL);

        gen_data_file_format_type export_type =
            gen_data_config_get_output_format(gen_data->config);
        gen_data_export(gen_data, full_path, export_type);
        free(full_path);
    }
}

static void gen_data_assert_index(const gen_data_type *gen_data, int index) {
    int current_size = gen_data_config_get_data_size(
        gen_data->config, gen_data->current_report_step);
    if ((index < 0) || (index >= current_size))
        util_abort("%s: index:%d invalid. Valid range: [0,%d) \n", __func__,
                   index, current_size);
}

double gen_data_iget_double(const gen_data_type *gen_data, int index) {
    gen_data_assert_index(gen_data, index);
    {
        ecl_data_type internal_type =
            gen_data_config_get_internal_data_type(gen_data->config);
        if (ecl_type_is_double(internal_type)) {
            double *data = (double *)gen_data->data;
            return data[index];
        } else {
            float *data = (float *)gen_data->data;
            return data[index];
        }
    }
}

void gen_data_export_data(const gen_data_type *gen_data,
                          double_vector_type *export_data) {
    ecl_data_type internal_type =
        gen_data_config_get_internal_data_type(gen_data->config);
    if (ecl_type_is_double(internal_type))
        double_vector_memcpy_from_data(export_data,
                                       (const double *)gen_data->data,
                                       gen_data_get_size(gen_data));
    else {
        double_vector_reset(export_data);
        float *float_data = (float *)gen_data->data;
        for (int i = 0; i < gen_data_get_size(gen_data); i++)
            double_vector_iset(export_data, i, float_data[i]);
    }
}

/**
   The filesystem will (currently) store gen_data instances which do
   not hold any data. Therefore it will be quite common to enter this
   function with an empty instance, we therefore just set valid =>
   false, and return silently in that case.
*/
C_USED bool gen_data_user_get(const gen_data_type *gen_data,
                              const char *index_key, int report_step,
                              double *value) {
    int index;
    *value = 0.0;

    if (index_key != NULL) {
        if (util_sscanf_int(index_key, &index)) {
            if (index < gen_data_config_get_data_size(
                            gen_data->config, gen_data->current_report_step)) {
                *value = gen_data_iget_double(gen_data, index);
                return true;
            }
        }
    }

    return false;
}

const char *gen_data_get_key(const gen_data_type *gen_data) {
    return gen_data_config_get_key(gen_data->config);
}

C_USED void gen_data_clear(gen_data_type *gen_data) {
    const gen_data_config_type *config = gen_data->config;
    ecl_data_type internal_type =
        gen_data_config_get_internal_data_type(config);
    const int data_size = gen_data_config_get_data_size(
        gen_data->config, gen_data->current_report_step);

    if (ecl_type_is_float(internal_type)) {
        float *data = (float *)gen_data->data;
        for (int i = 0; i < data_size; i++)
            data[i] = 0;
    } else if (ecl_type_is_double(internal_type)) {
        double *data = (double *)gen_data->data;
        for (int i = 0; i < data_size; i++)
            data[i] = 0;
    }
}

void gen_data_copy_to_double_vector(const gen_data_type *gen_data,
                                    double_vector_type *vector) {
    const ecl_data_type internal_type =
        gen_data_config_get_internal_data_type(gen_data->config);
    int size = gen_data_get_size(gen_data);
    if (ecl_type_is_float(internal_type)) {
        float *data = (float *)gen_data->data;
        double_vector_reset(vector);
        for (int i = 0; i < size; i++) {
            double_vector_append(vector, data[i]);
        }
    } else if (ecl_type_is_double(internal_type)) {
        double *data = (double *)gen_data->data;
        double_vector_memcpy_from_data(vector, data, size);
    }
}

UTIL_SAFE_CAST_FUNCTION_CONST(gen_data, GEN_DATA)
UTIL_SAFE_CAST_FUNCTION(gen_data, GEN_DATA)
VOID_USER_GET(gen_data)
VOID_ALLOC(gen_data)
VOID_FREE(gen_data)
VOID_COPY(gen_data)
VOID_INITIALIZE(gen_data)
VOID_ECL_WRITE(gen_data)
VOID_FORWARD_LOAD(gen_data)
VOID_READ_FROM_BUFFER(gen_data);
VOID_WRITE_TO_BUFFER(gen_data);
VOID_SERIALIZE(gen_data)
VOID_DESERIALIZE(gen_data)
VOID_CLEAR(gen_data)

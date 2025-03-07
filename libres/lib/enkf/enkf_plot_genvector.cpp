/*
   Copyright (C) 2014  Equinor ASA, Norway.

   The file 'enkf_plot_genvector.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <time.h>

#include <ert/util/vector.h>

#include <ert/enkf/enkf_fs.hpp>
#include <ert/enkf/enkf_plot_genvector.hpp>
#include <ert/enkf/gen_data.hpp>

#define ENKF_PLOT_GENVECTOR_TYPE_ID 66862669

struct enkf_plot_genvector_struct {
    UTIL_TYPE_ID_DECLARATION;
    int iens;
    double_vector_type *data;
    const enkf_config_node_type *config_node;
};

UTIL_IS_INSTANCE_FUNCTION(enkf_plot_genvector, ENKF_PLOT_GENVECTOR_TYPE_ID)

enkf_plot_genvector_type *
enkf_plot_genvector_alloc(const enkf_config_node_type *config_node, int iens) {
    enkf_plot_genvector_type *vector =
        (enkf_plot_genvector_type *)util_malloc(sizeof *vector);
    UTIL_TYPE_ID_INIT(vector, ENKF_PLOT_GENVECTOR_TYPE_ID);
    vector->config_node = config_node;
    vector->data = double_vector_alloc(0, 0);
    vector->iens = iens;
    return vector;
}

void enkf_plot_genvector_free(enkf_plot_genvector_type *vector) {
    double_vector_free(vector->data);
    free(vector);
}

int enkf_plot_genvector_get_size(const enkf_plot_genvector_type *vector) {
    return double_vector_size(vector->data);
}

double enkf_plot_genvector_iget(const enkf_plot_genvector_type *vector,
                                int index) {
    return double_vector_iget(vector->data, index);
}

void enkf_plot_genvector_load(enkf_plot_genvector_type *vector,
                              enkf_fs_type *fs, int report_step) {
    enkf_node_type *work_node = enkf_node_alloc(vector->config_node);

    node_id_type node_id = {.report_step = report_step, .iens = vector->iens};

    if (enkf_node_try_load(work_node, fs, node_id)) {
        const gen_data_type *node =
            (const gen_data_type *)enkf_node_value_ptr(work_node);
        gen_data_copy_to_double_vector(node, vector->data);
    }
    enkf_node_free(work_node);
}

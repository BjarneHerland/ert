/*
   Copyright (C) 2014  Equinor ASA, Norway.

   The file 'enkf_plot_gen_kw.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_ENKF_PLOT_GEN_KW_H
#define ERT_ENKF_PLOT_GEN_KW_H

#include <ert/util/bool_vector.h>
#include <ert/util/stringlist.h>
#include <ert/util/type_macros.h>

#include <ert/enkf/enkf_config_node.hpp>
#include <ert/enkf/enkf_plot_gen_kw_vector.hpp>

typedef struct enkf_plot_gen_kw_struct enkf_plot_gen_kw_type;

extern "C" enkf_plot_gen_kw_type *
enkf_plot_gen_kw_alloc(const enkf_config_node_type *enkf_config_node);
extern "C" void enkf_plot_gen_kw_free(enkf_plot_gen_kw_type *gen_kw);
extern "C" int enkf_plot_gen_kw_get_size(const enkf_plot_gen_kw_type *gen_kw);
extern "C" enkf_plot_gen_kw_vector_type *
enkf_plot_gen_kw_iget(const enkf_plot_gen_kw_type *vector, int index);
extern "C" void enkf_plot_gen_kw_load(enkf_plot_gen_kw_type *gen_kw,
                                      enkf_fs_type *fs, bool transform_data,
                                      int report_step,
                                      const bool_vector_type *input_mask);

extern "C" const char *
enkf_plot_gen_kw_iget_key(const enkf_plot_gen_kw_type *plot_gen_kw, int index);
int enkf_plot_gen_kw_get_keyword_index(const enkf_plot_gen_kw_type *gen_kw,
                                       const std::string &keyword);
extern "C" int
enkf_plot_gen_kw_get_keyword_count(const enkf_plot_gen_kw_type *gen_kw);
extern "C" bool
enkf_plot_gen_kw_should_use_log_scale(const enkf_plot_gen_kw_type *gen_kw,
                                      int index);

UTIL_IS_INSTANCE_HEADER(enkf_plot_gen_kw);

#endif

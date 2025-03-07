/*
   Copyright (C) 2013  Equinor ASA, Norway.

   The file 'analysis_iter_config.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_ANALYSIS_ITER_CONFIG_H
#define ERT_ANALYSIS_ITER_CONFIG_H

#include <ert/config/config_content.hpp>
#include <ert/config/config_parser.hpp>

typedef struct analysis_iter_config_struct analysis_iter_config_type;

extern "C" void
analysis_iter_config_set_num_iterations(analysis_iter_config_type *config,
                                        int num_iterations);
extern "C" int analysis_iter_config_get_num_iterations(
    const analysis_iter_config_type *config);
void analysis_iter_config_set_num_retries_per_iteration(
    analysis_iter_config_type *config, int num_retries);
extern "C" int analysis_iter_config_get_num_retries_per_iteration(
    const analysis_iter_config_type *config);
extern "C" void
analysis_iter_config_set_case_fmt(analysis_iter_config_type *config,
                                  const char *case_fmt);
extern "C" PY_USED char *
analysis_iter_config_get_case_fmt(analysis_iter_config_type *config);
extern "C" analysis_iter_config_type *analysis_iter_config_alloc();
extern "C" PY_USED analysis_iter_config_type *
analysis_iter_config_alloc_full(const char *case_fmt, int num_iterations,
                                int num_iter_tries);
extern "C" void analysis_iter_config_free(analysis_iter_config_type *config);
const char *analysis_iter_config_iget_case(analysis_iter_config_type *config,
                                           int iter);
void analysis_iter_config_add_config_items(config_parser_type *config);
void analysis_iter_config_init(analysis_iter_config_type *iter_config,
                               const config_content_type *config);
extern "C" bool
analysis_iter_config_case_fmt_set(const analysis_iter_config_type *config);
extern "C" bool analysis_iter_config_num_iterations_set(
    const analysis_iter_config_type *config);

#endif

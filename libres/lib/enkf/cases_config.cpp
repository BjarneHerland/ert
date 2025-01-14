/*
   Copyright (C) 2013  Equinor ASA, Norway.

   The file 'cases_config.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <filesystem>

#include <stdlib.h>
#include <string.h>

#include <ert/res_util/file_utils.hpp>
#include <ert/util/util.h>

#include <ert/enkf/cases_config.hpp>

namespace fs = std::filesystem;

struct cases_config_struct {
    int iteration_number;
};

cases_config_type *cases_config_alloc() {
    cases_config_type *config =
        (cases_config_type *)util_malloc(sizeof *config);
    config->iteration_number = 0;
    return config;
}

static void cases_config_set_iteration_number(cases_config_type *config,
                                              int num_iterations) {
    config->iteration_number = num_iterations;
}

int cases_config_get_iteration_number(const cases_config_type *config) {
    return config->iteration_number;
}

bool cases_config_set_int(cases_config_type *cases_config, const char *var_name,
                          int value) {
    bool name_recognized = true;
    if (strcmp(var_name, "iteration_number") == 0)
        cases_config_set_iteration_number(cases_config, value);
    else
        name_recognized = false;

    return name_recognized;
}

void cases_config_fwrite(cases_config_type *config, const char *filename) {
    auto stream = mkdir_fopen(fs::path(filename), "w");
    int iteration_no = cases_config_get_iteration_number(config);
    util_fwrite_int(iteration_no, stream);
    fclose(stream);
}

void cases_config_fread(cases_config_type *config, const char *filename) {
    if (fs::exists(filename)) {
        FILE *stream = util_fopen(filename, "r");
        int iteration_number = util_fread_int(stream);
        cases_config_set_iteration_number(config, iteration_number);
        fclose(stream);
    }
}

void cases_config_free(cases_config_type *config) { free(config); }

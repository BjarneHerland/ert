/*
   Copyright (C) 2013  Equinor ASA, Norway.

   The file 'enkf_gen_obs_load.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/util/test_util.h>

#include <ert/enkf/ert_test_context.hpp>
#include <ert/enkf/gen_data_config.hpp>

void test_obs_check_report_steps(const char *config_file) {
    ert_test_context_type *test_context =
        ert_test_context_alloc("GEN_OBS", config_file);
    enkf_main_type *enkf_main = ert_test_context_get_main(test_context);
    enkf_obs_type *obs = enkf_main_get_obs(enkf_main);

    test_assert_true(enkf_obs_has_key(obs, "POLY_OBS"));
    test_assert_false(enkf_obs_has_key(obs, "GEN_OBS30"));

    ert_test_context_free(test_context);
}

int main(int argc, char **argv) {
    const char *config_file = argv[1];

    test_obs_check_report_steps(config_file);
}

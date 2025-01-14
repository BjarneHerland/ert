/*
   Copyright (C) 2014  Equinor ASA, Norway.

   The file 'gen_kw_test.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <fstream>
#include <iostream>

#include <filesystem>

#include <stdio.h>
#include <stdlib.h>

#include <ert/enkf/ert_test_context.hpp>
#include <ert/util/test_util.h>
#include <ert/util/vector.hpp>

#include <ert/enkf/gen_kw_config.hpp>

namespace fs = std::filesystem;

namespace enkf_main {
void ecl_write(const ensemble_config_type *ens_config,
               const char *export_base_name, const run_arg_type *run_arg,
               enkf_fs_type *fs);
} // namespace enkf_main

void test_write_gen_kw_export_file(enkf_main_type *enkf_main) {
    std::vector<std::string> key_list = ensemble_config_keylist_from_var_type(
        enkf_main_get_ensemble_config(enkf_main), PARAMETER);
    enkf_state_type *state = enkf_main_iget_state(enkf_main, 0);
    enkf_fs_type *init_fs = enkf_main_get_fs(enkf_main);
    const subst_list_type *subst_list =
        subst_config_get_subst_list(enkf_main_get_subst_config(enkf_main));
    run_arg_type *run_arg = run_arg_alloc_INIT_ONLY(
        "run_id", init_fs, 0, 0, "simulations/run0", subst_list);
    rng_manager_type *rng_manager = enkf_main_get_rng_manager(enkf_main);
    rng_type *rng = rng_manager_iget(rng_manager, run_arg_get_iens(run_arg));
    enkf_state_initialize(state, rng, init_fs, key_list, INIT_FORCE);
    enkf_main::ecl_write(enkf_main_get_ensemble_config(enkf_main),
                         model_config_get_gen_kw_export_name(
                             enkf_main_get_model_config(enkf_main)),
                         run_arg, init_fs);
    test_assert_true(fs::exists("simulations/run0/parameters.txt"));
    test_assert_true(fs::exists("simulations/run0/parameters.json"));
    run_arg_free(run_arg);
}

static void read_erroneous_gen_kw_file(void *arg) {
    vector_type *arg_vector = vector_safe_cast(arg);
    gen_kw_config_type *gen_kw_config =
        (gen_kw_config_type *)vector_iget(arg_vector, 0);
    const char *filename = (const char *)vector_iget_const(arg_vector, 1);
    gen_kw_config_set_parameter_file(gen_kw_config, filename);
}

void test_read_erroneous_gen_kw_file() {
    const char *parameter_filename = "MULTFLT_with_errors.txt";
    const char *tmpl_filename = "MULTFLT.tmpl";

    {
        std::ofstream param_file(parameter_filename);
        param_file << "MULTFLT1 NORMAL 0\nMULTFLT2 RAW\nMULTFLT3 NORMAL 0";
        param_file.close();

        std::ofstream tmpl_file(tmpl_filename);
        tmpl_file << "MULTFLT1 NORMAL 0\nMULTFLT2 RAW\nMULTFLT3 NORMAL 0";
        tmpl_file.close();
    }

    gen_kw_config_type *gen_kw_config =
        gen_kw_config_alloc_empty("MULTFLT", "<%s>");
    vector_type *arg = vector_alloc_new();
    vector_append_ref(arg, gen_kw_config);
    vector_append_ref(arg, parameter_filename);

    test_assert_util_abort("gen_kw_config_set_parameter_file",
                           read_erroneous_gen_kw_file, arg);

    vector_free(arg);
    gen_kw_config_free(gen_kw_config);
}

int main(int argc, char **argv) {
    const char *config_file = argv[1];
    ert_test_context_type *test_context =
        ert_test_context_alloc("gen_kw_test", config_file);
    enkf_main_type *enkf_main = ert_test_context_get_main(test_context);
    test_assert_not_NULL(enkf_main);

    test_write_gen_kw_export_file(enkf_main);
    test_read_erroneous_gen_kw_file();
    ert_test_context_free(test_context);
    exit(0);
}

/*
   Copyright (C) 2012  Equinor ASA, Norway.

   The file 'config_include_test.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <stdio.h>
#include <stdlib.h>

#include <ert/util/path_stack.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <ert/config/config_parser.hpp>

void parse_test(
    config_parser_type *config,
    const char *
        root_path, // The new working directory - the test will start by chdir() here.
    const char *
        config_file) { // The config_file, either as an absolute path - or relative from root_path

    const char *path0 = "PATH0";
    const char *path1 = "path/PATH1";
    const char *path2 = "path/PATH2";
    const char *path3 = "path/subpath/PATH3";
    const char *path4 = "path/subpath/subsubpath/PATH4";

    char *config_path, *config_rel_path, *config_abs_path;
    path_stack_type *path_stack = path_stack_alloc();

    util_alloc_file_components(config_file, &config_path, NULL, NULL);
    path_stack_push(path_stack, NULL);
    if (root_path != NULL)
        util_chdir(root_path);

    config_abs_path = util_alloc_abs_path(config_path);
    config_rel_path = util_alloc_rel_path(NULL, config_abs_path);

    {
        config_content_type *content =
            config_parse(config, config_file, "--", "INCLUDE", NULL, NULL,
                         CONFIG_UNRECOGNIZED_IGNORE, true);
        if (config_content_is_valid(content)) {

            char *relpath0 = util_alloc_filename(config_rel_path, path0, NULL);
            char *relpath1 = util_alloc_filename(config_rel_path, path1, NULL);
            char *relpath2 = util_alloc_filename(config_rel_path, path2, NULL);
            char *relpath3 = util_alloc_filename(config_rel_path, path3, NULL);
            char *relpath4 = util_alloc_filename(config_rel_path, path4, NULL);

            char *abspath0 = util_alloc_filename(config_abs_path, path0, NULL);
            char *abspath1 = util_alloc_filename(config_abs_path, path1, NULL);
            char *abspath2 = util_alloc_filename(config_abs_path, path2, NULL);
            char *abspath3 = util_alloc_filename(config_abs_path, path3, NULL);
            char *abspath4 = util_alloc_filename(config_abs_path, path4, NULL);

            test_assert_string_equal(
                config_content_get_value_as_relpath(content, "PATH0"),
                relpath0);
            test_assert_string_equal(
                config_content_get_value_as_relpath(content, "PATH1"),
                relpath1);
            test_assert_string_equal(
                config_content_get_value_as_relpath(content, "PATH2"),
                relpath2);
            test_assert_string_equal(
                config_content_get_value_as_relpath(content, "PATH3"),
                relpath3);
            test_assert_string_equal(
                config_content_get_value_as_relpath(content, "PATH4"),
                relpath4);

            test_assert_string_equal(
                config_content_get_value_as_abspath(content, "PATH0"),
                abspath0);
            test_assert_string_equal(
                config_content_get_value_as_abspath(content, "PATH1"),
                abspath1);
            test_assert_string_equal(
                config_content_get_value_as_abspath(content, "PATH2"),
                abspath2);
            test_assert_string_equal(
                config_content_get_value_as_abspath(content, "PATH3"),
                abspath3);
            test_assert_string_equal(
                config_content_get_value_as_abspath(content, "PATH4"),
                abspath4);

        } else {
            const config_error_type *error = config_content_get_errors(content);
            config_error_fprintf(error, true, stdout);
            test_error_exit("Hmm - parsing %s failed \n", config_file);
        }
        config_content_free(content);
    }
    path_stack_pop(path_stack);
}

int main(int argc, char **argv) {
    const char *abs_path = argv[1];
    const char *config_file = argv[2];
    char *abs_config_file = util_alloc_filename(abs_path, config_file, NULL);
    config_parser_type *config = config_alloc();

    {
        config_schema_item_type *schema_item;

        schema_item = config_add_schema_item(config, "PATH0", true);
        config_schema_item_set_argc_minmax(schema_item, 1, 1);
        config_schema_item_iset_type(schema_item, 0, CONFIG_PATH);

        schema_item = config_add_schema_item(config, "PATH1", true);
        config_schema_item_set_argc_minmax(schema_item, 1, 1);
        config_schema_item_iset_type(schema_item, 0, CONFIG_PATH);

        schema_item = config_add_schema_item(config, "PATH2", true);
        config_schema_item_set_argc_minmax(schema_item, 1, 1);
        config_schema_item_iset_type(schema_item, 0, CONFIG_PATH);

        schema_item = config_add_schema_item(config, "PATH3", true);
        config_schema_item_set_argc_minmax(schema_item, 1, 1);
        config_schema_item_iset_type(schema_item, 0, CONFIG_PATH);

        schema_item = config_add_schema_item(config, "PATH4", true);
        config_schema_item_set_argc_minmax(schema_item, 1, 1);
        config_schema_item_iset_type(schema_item, 0, CONFIG_PATH);
    }

    parse_test(config, abs_path, config_file);
    parse_test(config, abs_path, abs_config_file);
    parse_test(config, NULL, abs_config_file);
    parse_test(config, "../../", abs_config_file);

    config_free(config);
    exit(0);
}

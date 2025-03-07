/*
   Copyright (C) 2013  Equinor ASA, Norway.

   The file 'config_argc.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <signal.h>
#include <stdlib.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <ert/config/config_parser.hpp>

void install_SIGNALS(void) {
    signal(
        SIGSEGV,
        util_abort_signal); /* Segmentation violation, i.e. overwriting memory ... */
    signal(SIGINT, util_abort_signal); /* Control C */
    signal(
        SIGTERM,
        util_abort_signal); /* If killing the program with SIGTERM (the default kill signal) you will get a backtrace.
                                             Killing with SIGKILL (-9) will not give a backtrace.*/
}

int main(int argc, char **argv) {
    install_SIGNALS();
    {
        const char *argc_OK = argv[1];
        const char *argc_less = argv[2];
        const char *argc_more = argv[3];

        config_parser_type *config = config_alloc();
        config_schema_item_type *schema_item =
            config_add_schema_item(config, "ITEM", false);
        config_schema_item_set_argc_minmax(schema_item, 2, 2);

        {
            config_content_type *content =
                config_parse(config, argc_OK, "--", NULL, NULL, NULL,
                             CONFIG_UNRECOGNIZED_ERROR, true);
            test_assert_true(config_content_is_instance(content));
            test_assert_true(config_content_is_valid(content));
            config_content_free(content);
        }

        {
            config_content_type *content =
                config_parse(config, argc_less, "--", NULL, NULL, NULL,
                             CONFIG_UNRECOGNIZED_ERROR, true);
            test_assert_true(config_content_is_instance(content));
            test_assert_false(config_content_is_valid(content));

            {
                const config_error_type *config_error =
                    config_content_get_errors(content);
                const char *error_msg =
                    "Error when parsing config_file:\"argc_less\" Keyword:ITEM "
                    "must have at least 2 arguments.";

                test_assert_int_equal(config_error_count(config_error), 1);
                test_assert_string_equal(config_error_iget(config_error, 0),
                                         error_msg);
            }
            config_content_free(content);
        }

        {
            config_content_type *content =
                config_parse(config, argc_more, "--", NULL, NULL, NULL,
                             CONFIG_UNRECOGNIZED_ERROR, true);
            test_assert_true(config_content_is_instance(content));
            test_assert_false(config_content_is_valid(content));
            {
                const config_error_type *config_error =
                    config_content_get_errors(content);
                const char *error_msg =
                    "Error when parsing config_file:\"argc_more\" Keyword:ITEM "
                    "must have maximum 2 arguments.";

                test_assert_int_equal(config_error_count(config_error), 1);
                test_assert_string_equal(config_error_iget(config_error, 0),
                                         error_msg);
            }
            config_content_free(content);
        }

        config_free(config);
        exit(0);
    }
}

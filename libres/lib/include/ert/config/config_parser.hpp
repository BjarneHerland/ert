/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'config.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_CONFIG_H
#define ERT_CONFIG_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <ert/util/hash.hpp>
#include <ert/util/stringlist.hpp>

#include <ert/res_util/subst_list.hpp>
#include <ert/tooling.hpp>

#include <ert/config/config_content.hpp>
#include <ert/config/config_content_item.hpp>
#include <ert/config/config_content_node.hpp>
#include <ert/config/config_schema_item.hpp>

typedef struct config_parser_struct config_parser_type;

extern "C" void config_free(config_parser_type *);
extern "C" config_parser_type *config_alloc();
extern "C" config_content_type *
config_parse(config_parser_type *config, const char *filename,
             const char *comment_string, const char *include_kw,
             const char *define_kw, const hash_type *pre_defined_kw_map,
             config_schema_unrecognized_enum unrecognized_behaviour,
             bool validate);
extern "C" bool config_has_schema_item(const config_parser_type *config,
                                       const char *kw);

extern "C" config_schema_item_type *
config_get_schema_item(const config_parser_type *, const char *);
void config_add_alias(config_parser_type *, const char *, const char *);
void config_install_message(config_parser_type *, const char *, const char *);

extern "C" config_schema_item_type *
config_add_schema_item(config_parser_type *config, const char *kw,
                       bool required);

extern "C" config_schema_item_type *
config_add_key_value(config_parser_type *config, const char *key, bool required,
                     config_item_types item_type);

extern "C" int config_get_schema_size(const config_parser_type *config);
void config_parser_deprecate(config_parser_type *config, const char *kw,
                             const char *msg);

extern "C" void config_validate(config_parser_type *config,
                                config_content_type *content);

extern "C" bool config_parser_add_key_values(
    config_parser_type *config, config_content_type *content, const char *kw,
    stringlist_type *values, const config_path_elm_type *current_path_elm,
    const char *config_filename, config_schema_unrecognized_enum unrecognized);

#endif

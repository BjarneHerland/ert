/*
   Copyright (C) 2012  Equinor ASA, Norway.

   The file 'config_content_item.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/util/stringlist.hpp>
#include <ert/util/vector.hpp>

#include <ert/config/config_content_item.hpp>
#include <ert/config/config_content_node.hpp>
#include <ert/config/config_schema_item.hpp>

#define CONFIG_CONTENT_ITEM_ID 8876752
struct config_content_item_struct {
    UTIL_TYPE_ID_DECLARATION;
    const config_schema_item_type *schema;
    vector_type *nodes;
    const config_path_elm_type *path_elm;
};

/**
   This function counts the number of times a config item has been
   set. Referring again to the example at the top:

     config_content_item_get_occurences( "KEY1" )

   will return 2.
*/
int config_content_item_get_size(const config_content_item_type *item) {
    return vector_get_size(item->nodes);
}

config_content_node_type *
config_content_item_get_last_node(const config_content_item_type *item) {
    return (config_content_node_type *)vector_get_last(item->nodes);
}

config_content_node_type *
config_content_item_iget_node(const config_content_item_type *item, int index) {
    return (config_content_node_type *)vector_iget(item->nodes, index);
}

const stringlist_type *
config_content_item_iget_stringlist_ref(const config_content_item_type *item,
                                        int occurence) {
    const config_content_node_type *node =
        config_content_item_iget_node(item, occurence);
    return config_content_node_get_stringlist(node);
}

/**
   If copy == false - the hash will break down when/if the
   config object is freed - your call.
*/
hash_type *config_content_item_alloc_hash(const config_content_item_type *item,
                                          bool copy) {
    hash_type *hash = hash_alloc();
    if (item != NULL) {
        int inode;
        for (inode = 0; inode < vector_get_size(item->nodes); inode++) {
            const config_content_node_type *node =
                config_content_item_iget_node(item, inode);
            const stringlist_type *src_list =
                config_content_node_get_stringlist(node);
            const char *key = stringlist_iget(src_list, 0);
            const char *value = stringlist_iget(src_list, 1);

            if (copy) {
                hash_insert_hash_owned_ref(hash, key,
                                           util_alloc_string_copy(value), free);
            } else
                hash_insert_ref(hash, key, value);
        }
    }
    return hash;
}

const char *config_content_item_iget(const config_content_item_type *item,
                                     int occurence, int index) {
    const config_content_node_type *node =
        config_content_item_iget_node(item, occurence);
    const stringlist_type *src_list = config_content_node_get_stringlist(node);
    return stringlist_iget(src_list, index);
}

bool config_content_item_iget_as_bool(const config_content_item_type *item,
                                      int occurence, int index) {
    bool value;
    config_schema_item_assure_type(item->schema, index, CONFIG_BOOL);
    util_sscanf_bool(config_content_item_iget(item, occurence, index), &value);
    return value;
}

int config_content_item_iget_as_int(const config_content_item_type *item,
                                    int occurence, int index) {
    int value;
    config_schema_item_assure_type(item->schema, index, CONFIG_INT);
    util_sscanf_int(config_content_item_iget(item, occurence, index), &value);
    return value;
}

double config_content_item_iget_as_double(const config_content_item_type *item,
                                          int occurence, int index) {
    double value;
    config_schema_item_assure_type(item->schema, index, CONFIG_FLOAT);
    util_sscanf_double(config_content_item_iget(item, occurence, index),
                       &value);
    return value;
}

/**
   Used to reset an item is the special string 'CLEAR_STRING'
   is found as the only argument:

   OPTION V1
   OPTION V2 V3 V4
   OPTION __RESET__
   OPTION V6

   In this case OPTION will get the value 'V6'. The example given
   above is a bit contrived; this option is designed for situations
   where several config files are parsed serially; and the user can
   not/will not update the first.
*/
void config_content_item_clear(config_content_item_type *item) {
    vector_clear(item->nodes);
}

void config_content_item_free(config_content_item_type *item) {
    vector_free(item->nodes);
    free(item);
}

UTIL_SAFE_CAST_FUNCTION(config_content_item, CONFIG_CONTENT_ITEM_ID)
UTIL_IS_INSTANCE_FUNCTION(config_content_item, CONFIG_CONTENT_ITEM_ID)

void config_content_item_free__(void *arg) {
    config_content_item_type *content_item = config_content_item_safe_cast(arg);
    config_content_item_free(content_item);
}

config_content_item_type *
config_content_item_alloc(const config_schema_item_type *schema,
                          const config_path_elm_type *path_elm) {
    config_content_item_type *content_item =
        (config_content_item_type *)util_malloc(sizeof *content_item);
    UTIL_TYPE_ID_INIT(content_item, CONFIG_CONTENT_ITEM_ID);
    content_item->schema = schema;
    content_item->nodes = vector_alloc_new();
    content_item->path_elm = path_elm;
    return content_item;
}

config_content_node_type *
config_content_item_alloc_node(const config_content_item_type *item,
                               const config_path_elm_type *path_elm) {
    config_content_node_type *node =
        config_content_node_alloc(item->schema, path_elm);
    vector_append_owned_ref(item->nodes, node, config_content_node_free__);
    return node;
}

const config_schema_item_type *
config_content_item_get_schema(const config_content_item_type *item) {
    return item->schema;
}

const config_path_elm_type *
config_content_item_get_path_elm(const config_content_item_type *item) {
    return item->path_elm;
}

/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'config_path_elm.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <string>
#include <vector>

#include <stdlib.h>

#include <ert/util/type_macros.hpp>

#include <ert/config/config_path_elm.hpp>
#include <ert/config/config_root_path.hpp>

#include <fmt/format.h>

#define CONFIG_PATH_ELM_TYPE_ID 7100063

struct config_path_elm_struct {
    UTIL_TYPE_ID_DECLARATION;
    /** This will always be absolute */
    char *abs_path;
    /** This will always be relative to the root path. */
    char *rel_path;
    const config_root_path_type *root_path;
};

static UTIL_SAFE_CAST_FUNCTION(config_path_elm, CONFIG_PATH_ELM_TYPE_ID)

    config_path_elm_type *config_path_elm_alloc(
        const config_root_path_type *root_path, const char *path) {
    if (root_path != NULL) {
        config_path_elm_type *path_elm =
            (config_path_elm_type *)util_malloc(sizeof *path_elm);
        UTIL_TYPE_ID_INIT(path_elm, CONFIG_PATH_ELM_TYPE_ID);
        path_elm->root_path = root_path;
        if (path == NULL) {
            path_elm->rel_path = NULL;
            path_elm->abs_path = util_alloc_string_copy(
                config_root_path_get_abs_path(root_path));
        } else {
            if (util_is_abs_path(path)) {
                path_elm->abs_path = util_alloc_string_copy(path);
                path_elm->rel_path = util_alloc_rel_path(
                    config_root_path_get_abs_path(root_path), path);
            } else {
                {
                    char *tmp_abs_path = (char *)util_alloc_filename(
                        config_root_path_get_abs_path(root_path), path, NULL);
                    path_elm->abs_path = util_alloc_abs_path(tmp_abs_path);
                    free(tmp_abs_path);
                }
                path_elm->rel_path = util_alloc_string_copy(path);
            }
        }
        return path_elm;
    } else {
        util_abort("%s: root_path input argument == NULL - invalid \n",
                   __func__);
        return NULL;
    }
}

void config_path_elm_free(config_path_elm_type *path_elm) {
    free(path_elm->rel_path);
    free(path_elm->abs_path);
    free(path_elm);
}

void config_path_elm_free__(void *arg) {
    config_path_elm_type *path_elm = config_path_elm_safe_cast(arg);
    config_path_elm_free(path_elm);
}

const char *config_path_elm_get_relpath(const config_path_elm_type *path_elm) {
    return path_elm->rel_path;
}

const char *config_path_elm_get_abspath(const config_path_elm_type *path_elm) {
    return path_elm->abs_path;
}

char *config_path_elm_alloc_path(const config_path_elm_type *path_elm,
                                 const char *path) {
    if (util_is_abs_path(path))
        return util_alloc_string_copy(path);
    else {
        /* This will be relative or absolute depending on the relative/absolute
       status of the root_path. */
        const char *input_root =
            config_root_path_get_input_path(path_elm->root_path);
        std::string tmp_path;
        char *return_path;
        if (input_root == NULL)
            tmp_path = util_alloc_filename(path_elm->rel_path, path, NULL);
        else {
            const std::vector<std::string> str_list{input_root,
                                                    path_elm->rel_path, path};

            tmp_path =
                fmt::format("{}", fmt::join(str_list, UTIL_PATH_SEP_STRING));
        }

        return_path = util_alloc_normal_path(tmp_path.c_str());

        return return_path;
    }
}

char *config_path_elm_alloc_relpath(const config_path_elm_type *path_elm,
                                    const char *input_path) {
    if (util_is_abs_path(input_path))
        return util_alloc_rel_path(
            config_root_path_get_rel_path(path_elm->root_path), input_path);
    else {
        char *abs_path = config_path_elm_alloc_abspath(path_elm, input_path);
        char *rel_path = (char *)util_alloc_rel_path(
            config_root_path_get_abs_path(path_elm->root_path), abs_path);
        free(abs_path);
        return rel_path;
    }
}

char *config_path_elm_alloc_abspath(const config_path_elm_type *path_elm,
                                    const char *input_path) {
    if (util_is_abs_path(input_path))
        return util_alloc_string_copy(input_path);
    else {
        char *abs_path1 =
            (char *)util_alloc_filename(path_elm->abs_path, input_path, NULL);
        char *abs_path = (char *)util_alloc_realpath__(
            abs_path1); // The util_alloc_realpath__() will work also for nonexsting paths
        free(abs_path1);
        return abs_path;
    }
}

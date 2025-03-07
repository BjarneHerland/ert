/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'ert_template.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <stdlib.h>

#include <ert/res_util/template.hpp>
#include <ert/util/hash.h>

#include <ert/enkf/config_keys.hpp>
#include <ert/enkf/ert_template.hpp>

#define ERT_TEMPLATE_TYPE_ID 7731963
#define ERT_TEMPLATES_TYPE_ID 6677330

/* Singular - one template. */
struct ert_template_struct {
    UTIL_TYPE_ID_DECLARATION;
    template_type *tmpl;
    char *target_file;
};

/* Plural - many templates. */
struct ert_templates_struct {
    UTIL_TYPE_ID_DECLARATION;
    subst_list_type *parent_subst;
    hash_type *templates;
};

void ert_template_set_target_file(ert_template_type *ert_template,
                                  const char *target_file) {
    ert_template->target_file =
        util_realloc_string_copy(ert_template->target_file, target_file);
}

void ert_template_set_template_file(ert_template_type *ert_template,
                                    const char *template_file) {
    template_set_template_file(ert_template->tmpl, template_file);
}

const char *
ert_template_get_template_file(const ert_template_type *ert_template) {
    return template_get_template_file(ert_template->tmpl);
}

const char *
ert_template_get_target_file(const ert_template_type *ert_template) {
    return ert_template->target_file;
}

ert_template_type *ert_template_alloc(const char *template_file,
                                      const char *target_file,
                                      subst_list_type *parent_subst) {
    ert_template_type *ert_template =
        (ert_template_type *)util_malloc(sizeof *ert_template);
    UTIL_TYPE_ID_INIT(ert_template, ERT_TEMPLATE_TYPE_ID);
    ert_template->tmpl = template_alloc(
        template_file, false,
        parent_subst); /* The templates are instantiated with internalize_template == false;
                          this means that substitutions are performed on the filename of the
                          template itself .*/

    ert_template->target_file = NULL;
    ert_template_set_target_file(ert_template, target_file);
    return ert_template;
}

void ert_template_free(ert_template_type *ert_template) {
    free(ert_template->target_file);
    template_free(ert_template->tmpl);
    free(ert_template);
}

void ert_template_instantiate(ert_template_type *ert_template, const char *path,
                              const subst_list_type *arg_list) {
    char *target_file =
        util_alloc_filename(path, ert_template->target_file, NULL);
    template_instantiate(ert_template->tmpl, target_file, arg_list, true);
    free(target_file);
}

void ert_template_add_arg(ert_template_type *ert_template, const char *key,
                          const char *value) {
    template_add_arg(ert_template->tmpl, key, value);
}

subst_list_type *ert_template_get_arg_list(ert_template_type *ert_template) {
    return template_get_args_list(ert_template->tmpl);
}

void ert_template_set_args_from_string(ert_template_type *ert_template,
                                       const char *arg_string) {
    template_clear_args(ert_template->tmpl);
    template_add_args_from_string(ert_template->tmpl, arg_string);
}

UTIL_SAFE_CAST_FUNCTION(ert_template, ERT_TEMPLATE_TYPE_ID)

void ert_template_free__(void *arg) {
    ert_template_free(ert_template_safe_cast(arg));
}

ert_templates_type *ert_templates_alloc_default(subst_list_type *parent_subst) {
    ert_templates_type *templates =
        (ert_templates_type *)util_malloc(sizeof *templates);
    UTIL_TYPE_ID_INIT(templates, ERT_TEMPLATES_TYPE_ID);
    templates->templates = hash_alloc();
    templates->parent_subst = parent_subst;
    return templates;
}

ert_templates_type *
ert_templates_alloc(subst_list_type *parent_subst,
                    const config_content_type *config_content) {

    ert_templates_type *templates = ert_templates_alloc_default(parent_subst);

    if (config_content)
        ert_templates_init(templates, config_content);

    return templates;
}

void ert_templates_free(ert_templates_type *ert_templates) {
    hash_free(ert_templates->templates);
    free(ert_templates);
}

void ert_templates_del_template(ert_templates_type *ert_templates,
                                const char *key) {
    hash_del(ert_templates->templates, key);
}

ert_template_type *ert_templates_add_template(ert_templates_type *ert_templates,
                                              const char *key,
                                              const char *template_file,
                                              const char *target_file,
                                              const char *arg_string) {
    ert_template_type *tmpl = ert_template_alloc(template_file, target_file,
                                                 ert_templates->parent_subst);
    ert_template_set_args_from_string(tmpl,
                                      arg_string); /* Arg_string can be NULL */

    /*
      If key == NULL the function will generate a key after the following algorithm:

      1. It tries with the basename of the template file.
      2. It tries with the basename of the template file, and a counter.
  */

    if (key == NULL) {
        char *new_key = NULL;
        char *base_name;
        int counter = 1;
        util_alloc_file_components(template_file, NULL, &base_name, NULL);
        do {
            if (counter == 1)
                new_key = util_realloc_string_copy(new_key, base_name);
            else
                new_key =
                    util_realloc_sprintf(new_key, "%s.%d", base_name, counter);
            counter++;
        } while (hash_has_key(ert_templates->templates, new_key));
        hash_insert_hash_owned_ref(ert_templates->templates, new_key, tmpl,
                                   ert_template_free__);
        free(new_key);
        free(base_name);
    } else
        hash_insert_hash_owned_ref(ert_templates->templates, key, tmpl,
                                   ert_template_free__);

    return tmpl;
}

void ert_templates_instansiate(ert_templates_type *ert_templates,
                               const char *path,
                               const subst_list_type *arg_list) {
    hash_iter_type *iter = hash_iter_alloc(ert_templates->templates);
    while (!hash_iter_is_complete(iter)) {
        ert_template_type *ert_template =
            (ert_template_type *)hash_iter_get_next_value(iter);
        ert_template_instantiate(ert_template, path, arg_list);
    }
    hash_iter_free(iter);
}

void ert_templates_clear(ert_templates_type *ert_templates) {
    hash_clear(ert_templates->templates);
}

ert_template_type *ert_templates_get_template(ert_templates_type *ert_templates,
                                              const char *key) {
    return (ert_template_type *)hash_get(ert_templates->templates, key);
}

stringlist_type *ert_templates_alloc_list(ert_templates_type *ert_templates) {
    return hash_alloc_stringlist(ert_templates->templates);
}

void ert_templates_init(ert_templates_type *templates,
                        const config_content_type *config) {
    if (config_content_has_item(config, RUN_TEMPLATE_KEY)) {
        const config_content_item_type *template_item =
            config_content_get_item(config, RUN_TEMPLATE_KEY);
        for (int i = 0; i < config_content_item_get_size(template_item); i++) {
            config_content_node_type *template_node =
                config_content_item_iget_node(template_item, i);
            const char *template_file =
                config_content_node_iget_as_abspath(template_node, 0);
            const char *target_file =
                config_content_node_iget(template_node, 1);

            ert_template_type *tmpl = ert_templates_add_template(
                templates, NULL, template_file, target_file, NULL);

            for (int iarg = 2;
                 iarg < config_content_node_get_size(template_node); iarg++) {
                char *key, *value;
                const char *key_value =
                    config_content_node_iget(template_node, iarg);
                util_binary_split_string(key_value, "=:", true, &key, &value);

                if (value != NULL)
                    ert_template_add_arg(tmpl, key, value);
                else
                    fprintf(
                        stderr,
                        "** Warning - failed to parse argument:%s as key:value "
                        "- ignored \n",
                        config_content_iget(config, "RUN_TEMPLATE", i, iarg));

                free(key);
                free(value);
            }
        }
    }
}

/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'analysis_config.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string>

#include <ert/util/util.h>

#include <ert/config/config_parser.hpp>
#include <ert/config/config_settings.hpp>

#include <ert/analysis/analysis_module.hpp>

#include <ert/enkf/analysis_config.hpp>
#include <ert/enkf/config_keys.hpp>
#include <ert/enkf/enkf_defaults.hpp>
#include <ert/enkf/model_config.hpp>
#include <ert/enkf/site_config.hpp>

#define UPDATE_ENKF_ALPHA_KEY "ENKF_ALPHA"
#define UPDATE_STD_CUTOFF_KEY "STD_CUTOFF"

#define ANALYSIS_CONFIG_TYPE_ID 64431306

using namespace std::string_literals;

struct analysis_config_struct {
    UTIL_TYPE_ID_DECLARATION;
    std::unordered_map<std::string, analysis_module_type *> analysis_modules;
    analysis_module_type *analysis_module;
    /** Points to directory with update logs. */
    char *log_path;

    /** Should we rerun the simulator when the parameters have been updated? */
    bool rerun;
    /** When rerunning - from where should we start? */
    int rerun_start;

    config_settings_type *update_settings;

    /** When creating the default ALL_ACTIVE local configuration. */
    bool single_node_update;
    analysis_iter_config_type *iter_config;
    int min_realisations;
    bool stop_long_running;
    int max_runtime;
    double global_std_scaling;
};

UTIL_IS_INSTANCE_FUNCTION(analysis_config, ANALYSIS_CONFIG_TYPE_ID)

/*

Interacting with modules
------------------------

It is possible to create a copy of an analysis module under a different
name, this can be convenient when trying out the same algorithm with
different parameter settings. I.e. based on the built in module STD_ENKF we
can create two copies with high and low truncation respectively:

   ANALYSIS_COPY  STD_ENKF  ENKF_HIGH_TRUNCATION
   ANALYSIS_COPY  STD_ENKF  ENKF_LOW_TRUNCATION

The copy operation does not differentiate between external and internal
modules. When a module has been loaded you can set internal parameters for
the module with the config command:

   ANALYSIS_SET_VAR  ModuleName  VariableName   Value

The module will be called with a function for setting variables which gets
the VariableName and value parameters as input; if the module recognizes
VariableName and Value is of the right type the module should set the
internal variable accordingly. If the module does not recognize the
variable name a warning will be printed on stderr, but no further action.

The actual analysis module to use is selected with the statement:

ANALYSIS_SELECT  ModuleName

[1] The libfile argument should include the '.so' extension, and can
    optionally contain a path component. The libfile will be passed directly to
    the dlopen() library call, this implies that normal runtime linking
    conventions apply - i.e. you have three options:

     1. The library name is given with a full path.
     2. The library is in a standard location for shared libraries.
     3. The library is in one of the directories mentioned in the
        LD_LIBRARY_PATH environment variable.

*/

void analysis_config_set_stop_long_running(analysis_config_type *config,
                                           bool stop_long_running) {
    config->stop_long_running = stop_long_running;
}

bool analysis_config_get_stop_long_running(const analysis_config_type *config) {
    return config->stop_long_running;
}

double
analysis_config_get_global_std_scaling(const analysis_config_type *config) {
    return config->global_std_scaling;
}

void analysis_config_set_global_std_scaling(analysis_config_type *config,
                                            double global_std_scaling) {
    config->global_std_scaling = global_std_scaling;
}

int analysis_config_get_max_runtime(const analysis_config_type *config) {
    return config->max_runtime;
}

void analysis_config_set_max_runtime(analysis_config_type *config,
                                     int max_runtime) {
    config->max_runtime = max_runtime;
}

void analysis_config_set_min_realisations(analysis_config_type *config,
                                          int min_realisations) {
    config->min_realisations = min_realisations;
}

int analysis_config_get_min_realisations(const analysis_config_type *config) {
    return config->min_realisations;
}

std::vector<std::string>
analysis_config_module_names(const analysis_config_type *config) {
    std::vector<std::string> s;

    for (const auto &analysis_pair : config->analysis_modules)
        s.push_back(analysis_pair.first);

    return s;
}

void analysis_config_set_alpha(analysis_config_type *config, double alpha) {
    config_settings_set_double_value(config->update_settings,
                                     UPDATE_ENKF_ALPHA_KEY, alpha);
}

double analysis_config_get_alpha(const analysis_config_type *config) {
    return config_settings_get_double_value(config->update_settings,
                                            UPDATE_ENKF_ALPHA_KEY);
}

void analysis_config_set_std_cutoff(analysis_config_type *config,
                                    double std_cutoff) {
    config_settings_set_double_value(config->update_settings,
                                     UPDATE_STD_CUTOFF_KEY, std_cutoff);
}

double analysis_config_get_std_cutoff(const analysis_config_type *config) {
    return config_settings_get_double_value(config->update_settings,
                                            UPDATE_STD_CUTOFF_KEY);
}
void analysis_config_set_log_path(analysis_config_type *config,
                                  const char *log_path) {
    config->log_path = util_realloc_string_copy(config->log_path, log_path);
}

/**
   Will in addition create the path.
*/
const char *analysis_config_get_log_path(const analysis_config_type *config) {
    util_make_path(config->log_path);
    return config->log_path;
}

void analysis_config_set_rerun_start(analysis_config_type *config,
                                     int rerun_start) {
    config->rerun_start = rerun_start;
}

void analysis_config_set_rerun(analysis_config_type *config, bool rerun) {
    config->rerun = rerun;
}

bool analysis_config_get_rerun(const analysis_config_type *config) {
    return config->rerun;
}

void analysis_config_set_single_node_update(analysis_config_type *config,
                                            bool single_node_update) {
    config->single_node_update = single_node_update;
}

bool analysis_config_get_single_node_update(
    const analysis_config_type *config) {
    return config->single_node_update;
}

int analysis_config_get_rerun_start(const analysis_config_type *config) {
    return config->rerun_start;
}

void analysis_config_load_module(analysis_config_type *config,
                                 analysis_mode_enum mode) {
    analysis_module_type *module = analysis_module_alloc(mode);
    if (module)
        config->analysis_modules[analysis_module_get_name(module)] = module;
    else
        fprintf(stderr, "** Warning: failed to create module \n");
}

void analysis_config_add_module_copy(analysis_config_type *config,
                                     const char *src_name,
                                     const char *target_name) {
    const analysis_module_type *src_module =
        analysis_config_get_module(config, src_name);
    analysis_module_type *target_module = analysis_module_alloc_named(
        analysis_module_get_mode(src_module), target_name);
    config->analysis_modules[target_name] = target_module;
}

analysis_module_type *
analysis_config_get_module(const analysis_config_type *config,
                           const char *module_name) {
    if (analysis_config_has_module(config, module_name)) {
        return config->analysis_modules.at(module_name);
    } else {
        throw std::invalid_argument(
            fmt::format("Analysis module named {} not found", module_name));
    }
}

bool analysis_config_has_module(const analysis_config_type *config,
                                const char *module_name) {
    return (config->analysis_modules.count(module_name) > 0);
}

bool analysis_config_select_module(analysis_config_type *config,
                                   const char *module_name) {
    if (analysis_config_has_module(config, module_name)) {
        analysis_module_type *module =
            analysis_config_get_module(config, module_name);

        if (analysis_module_get_name(module) == "IES_ENKF"s &&
            analysis_config_get_single_node_update(config)) {
            fprintf(stderr,
                    " ** Warning: the module:%s requires the setting "
                    "\"SINGLE_NODE_UPDATE FALSE\" in the config file.\n",
                    module_name);
            fprintf(stderr,
                    " **          the module has NOT been selected. \n");
            return false;
        }

        config->analysis_module = module;
        return true;
    } else {
        if (config->analysis_module == NULL)
            util_abort("%s: sorry module:%s does not exist - and no module "
                       "currently selected\n",
                       __func__, module_name);
        else
            fprintf(stderr,
                    "** Warning: analysis module:%s does not exist - current "
                    "selection unchanged:%s\n",
                    module_name,
                    analysis_module_get_name(config->analysis_module));
        return false;
    }
}

analysis_module_type *
analysis_config_get_active_module(const analysis_config_type *config) {
    return config->analysis_module;
}

const char *
analysis_config_get_active_module_name(const analysis_config_type *config) {
    if (config->analysis_module)
        return analysis_module_get_name(config->analysis_module);
    else
        return NULL;
}

void analysis_config_load_internal_modules(analysis_config_type *config) {
    analysis_config_load_module(config, ITERATED_ENSEMBLE_SMOOTHER);
    analysis_config_load_module(config, ENSEMBLE_SMOOTHER);
    analysis_config_select_module(config, DEFAULT_ANALYSIS_MODULE);
}

/**
   The analysis_config object is instantiated with the default values
   for enkf_defaults.h
*/
void analysis_config_init(analysis_config_type *analysis,
                          const config_content_type *config) {
    config_settings_apply(analysis->update_settings, config);

    if (config_content_has_item(config, UPDATE_LOG_PATH_KEY))
        analysis_config_set_log_path(
            analysis, config_content_get_value(config, UPDATE_LOG_PATH_KEY));

    if (config_content_has_item(config, STD_CUTOFF_KEY))
        analysis_config_set_std_cutoff(
            analysis,
            config_content_get_value_as_double(config, STD_CUTOFF_KEY));

    if (config_content_has_item(config, ENKF_ALPHA_KEY))
        analysis_config_set_alpha(analysis, config_content_get_value_as_double(
                                                config, ENKF_ALPHA_KEY));

    if (config_content_has_item(config, ENKF_RERUN_KEY))
        analysis_config_set_rerun(
            analysis, config_content_get_value_as_bool(config, ENKF_RERUN_KEY));

    if (config_content_has_item(config, SINGLE_NODE_UPDATE_KEY))
        analysis_config_set_single_node_update(
            analysis,
            config_content_get_value_as_bool(config, SINGLE_NODE_UPDATE_KEY));

    if (config_content_has_item(config, RERUN_START_KEY))
        analysis_config_set_rerun_start(
            analysis, config_content_get_value_as_int(config, RERUN_START_KEY));

    int num_realizations =
        config_content_get_value_as_int(config, NUM_REALIZATIONS_KEY);
    if (config_content_has_item(config, MIN_REALIZATIONS_KEY)) {

        config_content_node_type *config_content =
            config_content_get_value_node(config, MIN_REALIZATIONS_KEY);
        char *min_realizations_string =
            config_content_node_alloc_joined_string(config_content, " ");

        int min_realizations = DEFAULT_ANALYSIS_MIN_REALISATIONS;
        double percent = 0.0;
        if (util_sscanf_percent(min_realizations_string, &percent)) {
            min_realizations = ceil(num_realizations * percent / 100);
        } else {
            bool min_realizations_int_exists =
                util_sscanf_int(min_realizations_string, &min_realizations);
            if (!min_realizations_int_exists)
                fprintf(stderr,
                        "Method %s: failed to read integer value for "
                        "MIN_REALIZATIONS_KEY\n",
                        __func__);
        }

        if (min_realizations > num_realizations || min_realizations == 0)
            min_realizations = num_realizations;

        analysis_config_set_min_realisations(analysis, min_realizations);
        free(min_realizations_string);
    } else {
        analysis_config_set_min_realisations(analysis, num_realizations);
    }

    if (config_content_has_item(config, STOP_LONG_RUNNING_KEY))
        analysis_config_set_stop_long_running(
            analysis,
            config_content_get_value_as_bool(config, STOP_LONG_RUNNING_KEY));

    if (config_content_has_item(config, MAX_RUNTIME_KEY)) {
        analysis_config_set_max_runtime(
            analysis, config_content_get_value_as_int(config, MAX_RUNTIME_KEY));
    }

    /* Reload/copy modules. */
    {
        if (config_content_has_item(config, ANALYSIS_COPY_KEY)) {
            const config_content_item_type *copy_item =
                config_content_get_item(config, ANALYSIS_COPY_KEY);
            for (int i = 0; i < config_content_item_get_size(copy_item); i++) {
                const config_content_node_type *copy_node =
                    config_content_item_iget_node(copy_item, i);
                const char *src_name = config_content_node_iget(copy_node, 0);
                const char *target_name =
                    config_content_node_iget(copy_node, 1);

                analysis_config_add_module_copy(analysis, src_name,
                                                target_name);
            }
        }
    }

    /* Setting variables for analysis modules */
    {
        if (config_content_has_item(config, ANALYSIS_SET_VAR_KEY)) {
            const config_content_item_type *assign_item =
                config_content_get_item(config, ANALYSIS_SET_VAR_KEY);
            for (int i = 0; i < config_content_item_get_size(assign_item);
                 i++) {
                const config_content_node_type *assign_node =
                    config_content_item_iget_node(assign_item, i);

                const char *module_name =
                    config_content_node_iget(assign_node, 0);
                const char *var_name = config_content_node_iget(assign_node, 1);
                analysis_module_type *module;
                try {
                    module = analysis_config_get_module(analysis, module_name);
                } catch (std::invalid_argument &e) {
                    util_abort("Error loading config file: %s", e.what());
                }
                {
                    char *value = NULL;

                    for (int j = 2;
                         j < config_content_node_get_size(assign_node); j++) {
                        const char *config_value =
                            config_content_node_iget(assign_node, j);
                        if (value == NULL)
                            value = util_alloc_string_copy(config_value);
                        else {
                            value = util_strcat_realloc(value, " ");
                            value = util_strcat_realloc(value, config_value);
                        }
                    }

                    analysis_module_set_var(module, var_name, value);
                    free(value);
                }
            }
        }
    }

    if (config_content_has_item(config, ANALYSIS_SELECT_KEY))
        analysis_config_select_module(
            analysis, config_content_get_value(config, ANALYSIS_SELECT_KEY));

    analysis_iter_config_init(analysis->iter_config, config);
}

analysis_iter_config_type *
analysis_config_get_iter_config(const analysis_config_type *config) {
    return config->iter_config;
}

void analysis_config_free(analysis_config_type *config) {
    analysis_iter_config_free(config->iter_config);
    for (auto &module_pair : config->analysis_modules)
        analysis_module_free(module_pair.second);

    config_settings_free(config->update_settings);
    free(config->log_path);

    delete config;
}

analysis_config_type *analysis_config_alloc_full(
    double alpha, bool rerun, int rerun_start, const char *log_path,
    double std_cutoff, bool stop_long_running, bool single_node_update,
    double global_std_scaling, int max_runtime, int min_realisations) {
    analysis_config_type *config = new analysis_config_type();
    UTIL_TYPE_ID_INIT(config, ANALYSIS_CONFIG_TYPE_ID);

    config->log_path = NULL;
    config->update_settings = config_settings_alloc(UPDATE_SETTING_KEY);
    config_settings_add_double_setting(config->update_settings,
                                       UPDATE_ENKF_ALPHA_KEY, alpha);
    config_settings_add_double_setting(config->update_settings,
                                       UPDATE_STD_CUTOFF_KEY, std_cutoff);

    config->rerun = rerun;
    config->rerun_start = rerun_start;
    config->log_path = util_realloc_string_copy(config->log_path, log_path);
    config->single_node_update = single_node_update;
    config->min_realisations = min_realisations;
    config->stop_long_running = stop_long_running;
    config->max_runtime = max_runtime;

    config->analysis_module = NULL;
    config->iter_config = analysis_iter_config_alloc();
    config->global_std_scaling = global_std_scaling;

    analysis_config_load_internal_modules(config);

    return config;
}

analysis_config_type *analysis_config_alloc_default(void) {
    analysis_config_type *config = new analysis_config_type();
    UTIL_TYPE_ID_INIT(config, ANALYSIS_CONFIG_TYPE_ID);

    config->update_settings = config_settings_alloc(UPDATE_SETTING_KEY);
    config_settings_add_double_setting(
        config->update_settings, UPDATE_ENKF_ALPHA_KEY, DEFAULT_ENKF_ALPHA);
    config_settings_add_double_setting(config->update_settings,
                                       UPDATE_STD_CUTOFF_KEY,
                                       DEFAULT_ENKF_STD_CUTOFF);

    analysis_config_set_rerun(config, DEFAULT_RERUN);
    analysis_config_set_rerun_start(config, DEFAULT_RERUN_START);
    analysis_config_set_single_node_update(config, DEFAULT_SINGLE_NODE_UPDATE);
    analysis_config_set_log_path(config, DEFAULT_UPDATE_LOG_PATH);

    analysis_config_set_min_realisations(config,
                                         DEFAULT_ANALYSIS_MIN_REALISATIONS);
    analysis_config_set_stop_long_running(config,
                                          DEFAULT_ANALYSIS_STOP_LONG_RUNNING);
    analysis_config_set_max_runtime(config, DEFAULT_MAX_RUNTIME);

    config->analysis_module = NULL;
    config->iter_config = analysis_iter_config_alloc();
    config->global_std_scaling = 1.0;
    return config;
}

analysis_config_type *analysis_config_alloc_load(const char *user_config_file) {
    config_parser_type *config_parser = config_alloc();
    config_content_type *config_content = NULL;
    if (user_config_file)
        config_content =
            model_config_alloc_content(user_config_file, config_parser);

    analysis_config_type *analysis_config =
        analysis_config_alloc(config_content);

    config_content_free(config_content);
    config_free(config_parser);

    return analysis_config;
}

analysis_config_type *
analysis_config_alloc(const config_content_type *config_content) {
    analysis_config_type *analysis_config = analysis_config_alloc_default();

    if (config_content) {
        analysis_config_load_internal_modules(analysis_config);
        analysis_config_init(analysis_config, config_content);
    }

    return analysis_config;
}

void analysis_config_add_config_items(config_parser_type *config) {
    config_schema_item_type *item;

    config_add_key_value(config, ENKF_ALPHA_KEY, false, CONFIG_FLOAT);
    config_add_key_value(config, STD_CUTOFF_KEY, false, CONFIG_FLOAT);
    config_settings_init_parser__(UPDATE_SETTING_KEY, config, false);

    config_add_key_value(config, SINGLE_NODE_UPDATE_KEY, false, CONFIG_BOOL);

    config_add_key_value(config, ENKF_RERUN_KEY, false, CONFIG_BOOL);
    config_add_key_value(config, RERUN_START_KEY, false, CONFIG_INT);
    config_add_key_value(config, UPDATE_LOG_PATH_KEY, false, CONFIG_STRING);
    config_add_key_value(config, MIN_REALIZATIONS_KEY, false, CONFIG_STRING);
    config_add_key_value(config, MAX_RUNTIME_KEY, false, CONFIG_INT);

    item =
        config_add_key_value(config, STOP_LONG_RUNNING_KEY, false, CONFIG_BOOL);
    stringlist_type *child_list = stringlist_alloc_new();
    stringlist_append_copy(child_list, MIN_REALIZATIONS_KEY);
    config_schema_item_set_required_children_on_value(item, "TRUE", child_list);
    stringlist_free(child_list);

    config_add_key_value(config, ANALYSIS_SELECT_KEY, false, CONFIG_STRING);

    item = config_add_schema_item(config, ANALYSIS_COPY_KEY, false);
    config_schema_item_set_argc_minmax(item, 2, 2);

    item = config_add_schema_item(config, ANALYSIS_SET_VAR_KEY, false);
    config_schema_item_set_argc_minmax(item, 3, CONFIG_DEFAULT_ARG_MAX);
    analysis_iter_config_add_config_items(config);
}

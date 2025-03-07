/*
   Copyright (C) 2013  Equinor ASA, Norway.

   The file 'enkf_main_manage_fs.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <dirent.h>
#include <filesystem>
#include <string>
#include <vector>

#include <ert/enkf/enkf_defaults.hpp>
#include <ert/enkf/summary_key_set.hpp>
#include <ert/logging.hpp>
#include <ert/util/bool_vector.h>

namespace fs = std::filesystem;

bool enkf_main_case_is_current(const enkf_main_type *enkf_main,
                               const char *case_path) {
    char *mount_point = enkf_main_alloc_mount_point(enkf_main, case_path);
    const char *current_mount_point = NULL;
    bool is_current;

    if (enkf_main->dbase != NULL)
        current_mount_point = enkf_fs_get_mount_point(enkf_main->dbase);

    is_current = util_string_equal(mount_point, current_mount_point);
    free(mount_point);
    return is_current;
}

static bool
enkf_main_current_case_file_exists(const enkf_main_type *enkf_main) {
    const char *ens_path =
        model_config_get_enspath(enkf_main_get_model_config(enkf_main));
    char *current_case_file =
        util_alloc_filename(ens_path, CURRENT_CASE_FILE, NULL);
    bool exists = fs::exists(current_case_file);
    free(current_case_file);
    return exists;
}

char *enkf_main_read_alloc_current_case_name(const enkf_main_type *enkf_main) {
    char *current_case = NULL;
    const char *ens_path =
        model_config_get_enspath(enkf_main_get_model_config(enkf_main));
    char *current_case_file =
        util_alloc_filename(ens_path, CURRENT_CASE_FILE, NULL);
    if (enkf_main_current_case_file_exists(enkf_main)) {
        FILE *stream = util_fopen(current_case_file, "r");
        current_case = util_fscanf_alloc_token(stream);
        fclose(stream);
    } else {
        util_abort("%s: File: storage/current_case not found, aborting! \n",
                   __func__);
    }
    free(current_case_file);
    return current_case;
}

stringlist_type *enkf_main_alloc_caselist(const enkf_main_type *enkf_main) {
    stringlist_type *case_list = stringlist_alloc_new();
    {
        const char *ens_path =
            model_config_get_enspath(enkf_main_get_model_config(enkf_main));
        DIR *ens_dir = opendir(ens_path);
        if (ens_dir != NULL) {
            int ens_fd = dirfd(ens_dir);
            if (ens_fd != -1) {
                struct dirent *dp;
                do {
                    dp = readdir(ens_dir);
                    if (dp != NULL) {
                        if (!(util_string_equal(dp->d_name, ".") ||
                              util_string_equal(dp->d_name, ".."))) {
                            if (!util_string_equal(dp->d_name,
                                                   CURRENT_CASE_FILE)) {
                                char *full_path = util_alloc_filename(
                                    ens_path, dp->d_name, NULL);
                                if (util_is_directory(full_path))
                                    stringlist_append_copy(case_list,
                                                           dp->d_name);
                                free(full_path);
                            }
                        }
                    }
                } while (dp != NULL);
            }
        }
        closedir(ens_dir);
    }
    return case_list;
}

static void enkf_main_copy_ensemble(const ensemble_config_type *ensemble_config,
                                    enkf_fs_type *source_case_fs,
                                    int source_report_step,
                                    enkf_fs_type *target_case_fs,
                                    const std::vector<bool> &iens_mask,
                                    const std::vector<std::string> &node_list) {
    state_map_type *target_state_map = enkf_fs_get_state_map(target_case_fs);

    for (auto &node : node_list) {
        enkf_config_node_type *config_node =
            ensemble_config_get_node(ensemble_config, node.c_str());

        int src_iens = 0;
        for (auto mask : iens_mask) {
            if (mask) {
                node_id_type src_id = {.report_step = source_report_step,
                                       .iens = src_iens};
                node_id_type target_id = {.report_step = 0, .iens = src_iens};

                /* The copy is careful ... */
                if (enkf_config_node_has_node(config_node, source_case_fs,
                                              src_id))
                    enkf_node_copy(config_node, source_case_fs, target_case_fs,
                                   src_id, target_id);

                state_map_iset(target_state_map, src_iens, STATE_INITIALIZED);
            }
            src_iens++;
        }
    }
}

void enkf_main_init_current_case_from_existing(enkf_main_type *enkf_main,
                                               enkf_fs_type *source_case_fs,
                                               int source_report_step) {

    enkf_fs_type *current_fs = enkf_main_get_fs(enkf_main);

    enkf_main_init_case_from_existing(enkf_main, source_case_fs,
                                      source_report_step, current_fs);
}

void enkf_main_init_current_case_from_existing_custom(
    enkf_main_type *enkf_main, enkf_fs_type *source_case_fs,
    int source_report_step, std::vector<std::string> &node_list,
    std::vector<bool> &iactive) {

    enkf_fs_type *current_fs = enkf_main_get_fs(enkf_main);

    enkf_main_copy_ensemble(enkf_main_get_ensemble_config(enkf_main),
                            source_case_fs, source_report_step, current_fs,
                            iactive, node_list);
    enkf_fs_fsync(current_fs);
}

void enkf_main_init_case_from_existing(const enkf_main_type *enkf_main,
                                       enkf_fs_type *source_case_fs,
                                       int source_report_step,
                                       enkf_fs_type *target_case_fs) {

    std::vector<std::string> param_list = ensemble_config_keylist_from_var_type(
        enkf_main_get_ensemble_config(enkf_main),
        PARAMETER); /* Select only paramters - will fail for GEN_DATA of type DYNAMIC_STATE. */
    std::vector<bool> iactive(enkf_main_get_ensemble_size(enkf_main), true);
    enkf_main_copy_ensemble(enkf_main_get_ensemble_config(enkf_main),
                            source_case_fs, source_report_step, target_case_fs,
                            iactive, param_list);

    enkf_fs_fsync(target_case_fs);
}

/**
   This function will go through the filesystem and check that we have
   initial data for all parameters and all realizations. If the second
   argument mask is different from NULL, the function will only
   consider the realizations for which mask is true (if mask == NULL
   all realizations will be checked).
*/
static bool
enkf_main_case_is_initialized__(const ensemble_config_type *ensemble_config,
                                enkf_fs_type *fs, const int ens_size) {
    std::vector<std::string> parameter_keys =
        ensemble_config_keylist_from_var_type(ensemble_config, PARAMETER);
    bool initialized = true;
    for (int ikey = 0; (ikey < parameter_keys.size()) && initialized; ikey++) {
        const enkf_config_node_type *config_node = ensemble_config_get_node(
            ensemble_config, parameter_keys[ikey].c_str());
        initialized = enkf_config_node_has_node(config_node, fs,
                                                {.report_step = 0, .iens = 0});
        for (int iens = 0; (iens < ens_size) && initialized; iens++) {
            initialized = enkf_config_node_has_node(
                config_node, fs, {.report_step = 0, .iens = iens});
        }
    }

    return initialized;
}

bool enkf_main_case_is_initialized(const enkf_main_type *enkf_main,
                                   const char *case_name) {
    enkf_fs_type *fs = enkf_main_mount_alt_fs(enkf_main, case_name, false);
    if (fs) {
        bool initialized = enkf_main_case_is_initialized__(
            enkf_main_get_ensemble_config(enkf_main), fs, enkf_main->ens_size);
        enkf_fs_decref(fs);
        return initialized;
    } else
        return false;
}

static void update_case_log(enkf_main_type *enkf_main, const char *case_path) {
    /*  : Update a small text file with the name of the host currently
        running ert, the pid number of the process, the active case
        and when it started.

        If the previous shutdown was unclean the file will be around,
        and we will need the info from the previous invocation which
        is in the file. For that reason we open with mode 'a' instead
        of 'w'.
  */

    const char *ens_path =
        model_config_get_enspath(enkf_main_get_model_config(enkf_main));

    {
        int buffer_size = 256;
        char *current_host = util_alloc_filename(ens_path, CASE_LOG, NULL);
        FILE *stream = util_fopen(current_host, "a");

        fprintf(stream, "CASE:%-16s  ", case_path);
        fprintf(stream, "PID:%-8d  ", getpid());
        {
            char hostname[buffer_size];
            gethostname(hostname, buffer_size);
            fprintf(stream, "HOST:%-16s  ", hostname);
        }

        {
            int year, month, day, hour, minute, second;
            time_t now = time(NULL);

            util_set_datetime_values_utc(now, &second, &minute, &hour, &day,
                                         &month, &year);

            fprintf(stream, "TIME:%02d/%02d/%4d-%02d.%02d.%02d\n", day, month,
                    year, hour, minute, second);
        }
        fclose(stream);
        free(current_host);
    }
}

static void enkf_main_write_current_case_file(const enkf_main_type *enkf_main,
                                              const char *case_path) {
    const char *ens_path =
        model_config_get_enspath(enkf_main_get_model_config(enkf_main));
    const char *base = CURRENT_CASE_FILE;
    char *current_case_file = util_alloc_filename(ens_path, base, NULL);
    FILE *stream = util_fopen(current_case_file, "w");
    fprintf(stream, "%s", case_path);
    fclose(stream);
    free(current_case_file);
}

static void enkf_main_gen_data_special(enkf_main_type *enkf_main,
                                       enkf_fs_type *fs) {
    ensemble_config_type *ensemble_config =
        enkf_main_get_ensemble_config(enkf_main);
    stringlist_type *gen_data_keys =
        ensemble_config_alloc_keylist_from_impl_type(ensemble_config, GEN_DATA);
    for (int i = 0; i < stringlist_get_size(gen_data_keys); i++) {
        enkf_config_node_type *config_node = ensemble_config_get_node(
            ensemble_config, stringlist_iget(gen_data_keys, i));
        gen_data_config_type *gen_data_config =
            (gen_data_config_type *)enkf_config_node_get_ref(config_node);

        if (gen_data_config_is_dynamic(gen_data_config))
            gen_data_config_set_ens_size(gen_data_config, enkf_main->ens_size);
    }
    stringlist_free(gen_data_keys);
}

static void
enkf_main_update_current_case(enkf_main_type *enkf_main,
                              const char *case_path /* Can be NULL */) {
    if (!case_path)
        case_path = enkf_fs_get_case_name(enkf_main_get_fs(enkf_main));

    enkf_main_write_current_case_file(enkf_main, case_path);
    update_case_log(enkf_main, case_path);

    enkf_main_gen_data_special(enkf_main, enkf_main_get_fs(enkf_main));
    enkf_main_add_internal_subst_kw(enkf_main, "ERT-CASE",
                                    enkf_main_get_current_fs(enkf_main),
                                    "Current case");
    enkf_main_add_internal_subst_kw(enkf_main, "ERTCASE",
                                    enkf_main_get_current_fs(enkf_main),
                                    "Current case");
}

static void enkf_main_create_fs(const enkf_main_type *enkf_main,
                                const char *case_path) {
    char *new_mount_point = enkf_main_alloc_mount_point(enkf_main, case_path);

    enkf_fs_create_fs(new_mount_point, BLOCK_FS_DRIVER_ID, false);

    free(new_mount_point);
}

const char *enkf_main_get_mount_root(const enkf_main_type *enkf_main) {
    return model_config_get_enspath(enkf_main_get_model_config(enkf_main));
}

char *enkf_main_alloc_mount_point(const enkf_main_type *enkf_main,
                                  const char *case_path) {
    char *mount_point;
    if (util_is_abs_path(case_path))
        mount_point = util_alloc_string_copy(case_path);
    else
        mount_point = util_alloc_filename(
            model_config_get_enspath(enkf_main_get_model_config(enkf_main)),
            case_path, NULL);
    return mount_point;
}

/**
  Return a weak reference - i.e. the refcount is not increased.
*/
enkf_fs_type *enkf_main_get_fs(const enkf_main_type *enkf_main) {
    return enkf_main->dbase;
}

enkf_fs_type *enkf_main_job_get_fs(const enkf_main_type *enkf_main) {
    return enkf_main->dbase;
}

enkf_fs_type *enkf_main_get_fs_ref(const enkf_main_type *enkf_main) {
    return enkf_fs_get_ref(enkf_main->dbase);
}

const char *enkf_main_get_current_fs(const enkf_main_type *enkf_main) {
    return enkf_fs_get_case_name(enkf_main->dbase);
}

/**
  This function will return a valid enkf_fs instance; either just a
  pointer to the current enkf_main->dbase, or alternatively it will
  create a brand new fs instance. Because we do not really know whether
  a new instance has been created or not resource handling becomes
  slightly non trivial:


    1. When calling scope is finished with the enkf_fs instance it
       must call enkf_fs_decref(); the enkf_fs_decref() function will
       close the filesystem and free all resources when the reference
       count has reached zero.
*/
enkf_fs_type *enkf_main_mount_alt_fs(const enkf_main_type *enkf_main,
                                     const char *case_path, bool create) {
    if (enkf_main_case_is_current(enkf_main, case_path)) {
        // Fast path - we just return a reference to the currently selected case;
        // with increased refcount.
        enkf_fs_incref(enkf_main->dbase);
        return enkf_main->dbase;
    } else {
        // We have asked for an alterantive fs - must mount and possibly create that first.
        enkf_fs_type *new_fs = NULL;
        if (case_path != NULL) {
            char *new_mount_point =
                enkf_main_alloc_mount_point(enkf_main, case_path);

            if (!enkf_fs_exists(new_mount_point)) {
                if (create)
                    enkf_main_create_fs(enkf_main, case_path);
            }

            new_fs = enkf_fs_mount(new_mount_point);
            if (new_fs) {
                const model_config_type *model_config =
                    enkf_main_get_model_config(enkf_main);
                const ecl_sum_type *refcase =
                    model_config_get_refcase(model_config);

                if (refcase) {
                    time_map_type *time_map = enkf_fs_get_time_map(new_fs);
                    if (!time_map_attach_refcase(time_map, refcase))
                        logger->error("Warning mismatch between refcase:{} "
                                      "and existing case:{}",
                                      ecl_sum_get_case(refcase),
                                      new_mount_point);
                }
            }

            free(new_mount_point);
        }
        return new_fs;
    }
}

static void enkf_main_update_summary_config_from_fs__(enkf_main_type *enkf_main,
                                                      enkf_fs_type *fs) {
    ensemble_config_type *ensemble_config =
        enkf_main_get_ensemble_config(enkf_main);
    summary_key_set_type *summary_key_set = enkf_fs_get_summary_key_set(fs);
    stringlist_type *keys = summary_key_set_alloc_keys(summary_key_set);

    for (int i = 0; i < stringlist_get_size(keys); i++) {
        const char *key = stringlist_iget(keys, i);
        ensemble_config_add_summary(ensemble_config, key, LOAD_FAIL_SILENT);
    }
    stringlist_free(keys);
}

/**
   The enkf_fs instances employ a simple reference counting
   scheme. The main point with this system is to avoid opening the
   full timesystem more than necessary (this is quite compute
   intensive). This is essentially achieved by:

      1. Create new fs instances by using the function
         enkf_main_mount_alt_fs() - depending on the input arguments
         this will either create a new enkf_fs instance or it will
         just return a pointer to currently open fs instance; with an
         increased refcount.

      2. When you are finished with working with filesystem pointer
         call enkf_fs_unmount() - this will reduce the refcount with
         one, and eventually discard the complete datastructure when
         the refcount has reached zero.

      3. By using the function enkf_main_get_fs() /
         enkf_fs_get_weakref() you get a pointer to the current fs
         instance WITHOUT INCREASING THE REFCOUNT. This means that
         scope calling one of these functions does not get any
         ownership to the enkf_fs instance.

   The enkf_main instance will take ownership of the enkf_fs instance;
   this implies that the calling scope must have proper ownership of
   the fs instance which is passed in. The return value from
   enkf_main_get_fs() can NOT be used as input to this function; this
   is not checked for in any way - but the crash will be horrible if
   this is not adhered to.
*/
void enkf_main_set_fs(enkf_main_type *enkf_main, enkf_fs_type *fs,
                      const char *case_path /* Can be NULL */) {
    if (enkf_main->dbase != fs) {
        enkf_fs_incref(fs);

        if (enkf_main->dbase)
            enkf_fs_decref(enkf_main->dbase);

        enkf_main->dbase = fs;
        enkf_main_update_current_case(enkf_main, case_path);

        enkf_main_update_summary_config_from_fs__(enkf_main, fs);
    }
}

void enkf_main_select_fs(enkf_main_type *enkf_main, const char *case_path) {
    if (enkf_main_case_is_current(enkf_main, case_path))
        return; /* We have tried to select the currently selected case - just return. */
    else {
        enkf_fs_type *new_fs =
            enkf_main_mount_alt_fs(enkf_main, case_path, true);
        if (enkf_main->dbase == new_fs)
            util_abort("%s : return reference to current FS in situation where "
                       "that should not happen.\n",
                       __func__);

        if (new_fs != NULL)
            enkf_main_set_fs(enkf_main, new_fs, case_path);
        else {
            const char *ens_path =
                model_config_get_enspath(enkf_main_get_model_config(enkf_main));
            util_exit("%s: select filesystem %s:%s failed \n", __func__,
                      ens_path, case_path);
        }
        enkf_fs_decref(new_fs);
    }
}

static void enkf_main_user_select_initial_fs(enkf_main_type *enkf_main) {
    const char *ens_path =
        model_config_get_enspath(enkf_main_get_model_config(enkf_main));
    char *current_mount_point =
        util_alloc_filename(ens_path, CURRENT_CASE, NULL);

    if (enkf_main_current_case_file_exists(enkf_main)) {
        char *current_case = enkf_main_read_alloc_current_case_name(enkf_main);
        enkf_main_select_fs(enkf_main, current_case);
        free(current_case);
    } else if (enkf_fs_exists(current_mount_point) &&
               util_is_link(current_mount_point)) {
        /*If the current_case file does not exists, but the 'current' symlink does we use readlink to
    get hold of the actual target before calling the  enkf_main_select_fs() function. We then
    write the current_case file and delete the symlink.*/
        char *target_case = util_alloc_atlink_target(ens_path, CURRENT_CASE);
        enkf_main_select_fs(enkf_main, target_case);
        unlink(current_mount_point);
        enkf_main_write_current_case_file(enkf_main, target_case);
        free(target_case);
    } else
        enkf_main_select_fs(enkf_main,
                            DEFAULT_CASE); // Selecting (a new) default case

    free(current_mount_point);
}

bool enkf_main_fs_exists(const enkf_main_type *enkf_main,
                         const char *input_case) {
    bool exists = false;
    char *new_mount_point = enkf_main_alloc_mount_point(enkf_main, input_case);
    if (enkf_fs_exists(new_mount_point))
        exists = true;

    free(new_mount_point);
    return exists;
}

state_map_type *
enkf_main_alloc_readonly_state_map(const enkf_main_type *enkf_main,
                                   const char *case_path) {
    char *mount_point = enkf_main_alloc_mount_point(enkf_main, case_path);
    state_map_type *state_map = enkf_fs_alloc_readonly_state_map(mount_point);
    free(mount_point);
    return state_map;
}

void enkf_main_close_fs(enkf_main_type *enkf_main) {
    if (enkf_main->dbase != NULL)
        enkf_fs_decref(enkf_main->dbase);
}

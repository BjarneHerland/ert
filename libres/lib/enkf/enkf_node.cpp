/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'enkf_node.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <string.h>

#include <ert/util/buffer.h>
#include <ert/util/rng.h>
#include <ert/util/util.h>
#include <ert/util/vector.h>

#include <ert/enkf/container.hpp>
#include <ert/enkf/enkf_node.hpp>
#include <ert/enkf/ext_param.hpp>
#include <ert/enkf/field.hpp>
#include <ert/enkf/gen_data.hpp>
#include <ert/enkf/gen_kw.hpp>
#include <ert/enkf/summary.hpp>
#include <ert/enkf/surface.hpp>

#define ENKF_NODE_TYPE_ID 71043086

/**
   A small illustration (says more than thousand words ...) of how the
   enkf_node, enkf_config_node, field[1] and field_config[1] objects
   are linked.


   ================
   |              |   o-----------
   |  ================           |                =====================
   |  |              |   o--------                |                   |
   |  |  ================        |------------->  |                   |
   |  |  |              |        |                |  enkf_config_node |
   |  |  |              |        |                |                   |
   ===|  |  enkf_node   |  o------                |                   |
   o |  |              |                         |                   |
   | ===|              |                         =====================
   |  o |              |                                   o
   |  | ================                                   |
   |  |        o                                           |
   |  \        |                                           |
   |   \       |                                           |
   |    |      |                                           |
   |    |      |                                           |
   |    |      |                                           |
   |    |      |                                           |
   \|/   |      |                                           |
   ======|======|==                                        \|/
   |    \|/     | |   o-----------
   |  ==========|=====           |                =====================
   |  |        \|/   |   o--------                |                   |
   |  |  ================        |------------->  |                   |
   |  |  |              |        |                |  field_config     |
   |  |  |              |        |                |                   |
   ===|  |  field       |  o------                |                   |
   |     |              |                         |                   |
   ===   |              |                         =====================
         |              |
         ================


   To summarize in words:

   * The enkf_node object is an abstract object, which again contains
   a spesific enkf_object, like e.g. the field objects shown
   here. In general we have an ensemble of enkf_node objects.

   * The enkf_node objects contain a pointer to an enkf_config_node
   object.

   * The enkf_config_node object contains a pointer to the specific
   config object, i.e. field_config in this case.

   * All the field objects contain a pointer to a field_config object.


   [1]: field is just an example, and could be replaced with any of
   the enkf object types.

   A note on memory
   ================

   The enkf_nodes can consume large amounts of memory, and for large
   models/ensembles we have a situation where not all the
   members/fields can be in memory simultaneously - such low-memory
   situations are not really supported at the moment, but we have
   implemented some support for such problems:

   o All enkf objects should have a xxx_realloc_data() function. This
   function should be implemented in such a way that it is always
   safe to call, i.e. if the object already has allocated data the
   function should just return.

   o All enkf objects should implement a xxx_free_data()
   function. This function free the data of the object, and set the
   data pointer to NULL.


   The following 'rules' apply to the memory treatment:
   ----------------------------------------------------

   o Functions writing to memory can always be called, and it is their
   responsibility to allocate memory before actually writing on it. The
   writer functions are:

   enkf_node_initialize()
   enkf_node_forward_load()

   These functions should all start with a call to
   enkf_node_ensure_memory(). The (re)allocation of data is done at
   the enkf_node level, and **NOT** in the low level object
   (altough that is where it is eventually done of course).

   o When it comes to functions reading memory it is a bit more
   tricky. It could be that if the functions are called without
   memory, that just means that the object is not active or
   something (and the function should just return). On the other
   hand trying to read a NULL pointer does indicate that program
   logic is not fully up to it? And should therefore maybe be
   punished?

   o The only memory operation which is exported to 'user-space'
   (i.e. the enkf_state object) is enkf_node_free_data().

   Keeeping track of node state.
   =============================

   To keep track of the state of the node's data (actually the data of
   the contained enkf_object, i.e. a field) we have three highly
   internal variables __state, __modified , __iens, and
   __report_step. These three variables are used/updated in the
   following manner:



   1. The nodes are created with (modified, report_step, state, iens) ==
   (true , -1 , undefined , -1).

   2. After initialization we set: report_step -> 0 , state ->
   analyzed, modified -> true, iens -> -1

   3. After load (both from ensemble and ECLIPSE). We set modified ->
   false, and report_step, state and iens according to the load
   arguments.

   4. After deserialize (i.e. update) we set modified -> true.

   5. After write (to ensemble) we set in the same way as after load.

   6. After free_data we invalidate according to the newly allocated
   status.

   7. In the ens_load routine we check if modified == false and the
   report_step and state arguments agree with the current
   values. IN THAT CASE WE JUST RETURN WITHOUT ACTUALLY HITTING
   THE FILESYSTEM. This performance gain is the main point of the
   whole exercise.
*/
struct enkf_node_struct {
    UTIL_TYPE_ID_DECLARATION;
    alloc_ftype *alloc;
    ecl_write_ftype *ecl_write;
    forward_load_ftype *forward_load;
    forward_load_vector_ftype *forward_load_vector;
    free_data_ftype *free_data;
    user_get_ftype *user_get;
    user_get_vector_ftype *user_get_vector;
    fload_ftype *fload;
    has_data_ftype *has_data;

    serialize_ftype *serialize;
    deserialize_ftype *deserialize;
    read_from_buffer_ftype *read_from_buffer;
    write_to_buffer_ftype *write_to_buffer;
    initialize_ftype *initialize;
    node_free_ftype *freef;
    clear_ftype *clear;
    node_copy_ftype *copy;

    bool vector_storage;

    /** The (hash)key this node is identified with. */
    char *node_key;

    /** A pointer to the underlying enkf_object, i.e. gen_kw_type instance, or
     * a field_type instance or ... */
    void *data;
    /** A pointer to a enkf_config_node instance (which again cointans a
     * pointer to the config object of data). */
    const enkf_config_node_type *config;
    vector_type *container_nodes;
};

UTIL_IS_INSTANCE_FUNCTION(enkf_node, ENKF_NODE_TYPE_ID)

const enkf_config_node_type *enkf_node_get_config(const enkf_node_type *node) {
    return node->config;
}

bool enkf_node_vector_storage(const enkf_node_type *node) {
    return node->vector_storage;
}

/*
  All the function pointers REALLY should be in the config object ...
*/

#define FUNC_ASSERT(func)                                                      \
    if (func == NULL)                                                          \
        util_abort("%s: function handler: %s not registered for node:%s - "    \
                   "aborting\n",                                               \
                   __func__, #func, enkf_node->node_key);

void enkf_node_alloc_domain_object(enkf_node_type *enkf_node) {
    FUNC_ASSERT(enkf_node->alloc);
    {
        if (enkf_node->data != NULL)
            enkf_node->freef(enkf_node->data);
        enkf_node->data = enkf_node->alloc(
            enkf_config_node_get_ref(enkf_node->config)); // CXX_CAST_ERROR
    }
}

enkf_node_type *enkf_node_copyc(const enkf_node_type *enkf_node) {
    FUNC_ASSERT(enkf_node->copy);
    {
        const enkf_node_type *src = enkf_node;
        enkf_node_type *target;
        target = enkf_node_alloc(src->config);
        src->copy(src->data,
                  target->data); /* Calling the low level copy function */
        return target;
    }
}

ert_impl_type enkf_node_get_impl_type(const enkf_node_type *enkf_node) {
    return enkf_config_node_get_impl_type(enkf_node->config);
}

bool enkf_node_use_forward_init(const enkf_node_type *enkf_node) {
    return enkf_config_node_use_forward_init(enkf_node->config);
}

void *enkf_node_value_ptr(const enkf_node_type *enkf_node) {
    return enkf_node->data;
}

/**
   This function calls the node spesific ecl_write function. IF the
   ecl_file of the (node == NULL) *ONLY* the path is sent to the node
   spesific file.
*/
void enkf_node_ecl_write(const enkf_node_type *enkf_node, const char *path,
                         value_export_type *export_value, int report_step) {
    if (enkf_node->ecl_write != NULL) {
        char *node_eclfile = enkf_config_node_alloc_outfile(
            enkf_node->config,
            report_step); /* Will return NULL if the node does not have any outfile format. */
        /*
      If the node does not have a outfile (i.e. ecl_file), the
      ecl_write function will be called with file argument NULL. It
      is then the responsability of the low-level implementation to
      do "the right thing".
    */
        enkf_node->ecl_write(enkf_node->data, path, node_eclfile, export_value);
        free(node_eclfile);
    }
}

/**
   This function takes a string - key - as input an calls a node
   specific function to look up one scalar based on that key. The key
   is always a string, but the the type of content will vary for the
   different objects. For a field, the key will be a string of "i,j,k"
   for a cell.

   If the user has asked for something which does not exist the
   function SHOULD NOT FAIL; it should return false and set the *value
   to 0.
*/
bool enkf_node_user_get(enkf_node_type *enkf_node, enkf_fs_type *fs,
                        const char *key, node_id_type node_id, double *value) {
    return enkf_node_user_get_no_id(enkf_node, fs, key, node_id.report_step,
                                    node_id.iens, value);
}

bool enkf_node_user_get_no_id(enkf_node_type *enkf_node, enkf_fs_type *fs,
                              const char *key, int report_step, int iens,
                              double *value) {
    node_id_type node_id = {.report_step = report_step, .iens = iens};
    bool loadOK;
    FUNC_ASSERT(enkf_node->user_get);
    {
        loadOK = enkf_node_try_load(enkf_node, fs, node_id);

        if (loadOK)
            return enkf_node->user_get(enkf_node->data, key, report_step,
                                       value);
        else {
            *value = 0;
            return false;
        }
    }
}

bool enkf_node_user_get_vector(enkf_node_type *enkf_node, enkf_fs_type *fs,
                               const char *key, int iens,
                               double_vector_type *values) {
    if (enkf_node->vector_storage) {
        if (enkf_node_try_load_vector(enkf_node, fs, iens)) {
            enkf_node->user_get_vector(enkf_node->data, key, values);
            return true;
        } else
            return false;
    } else {
        util_abort("%s: internal error - function should only be called by "
                   "nodes with vector storage.\n",
                   __func__);
        return false;
    }
}

bool enkf_node_fload(enkf_node_type *enkf_node, const char *filename) {
    FUNC_ASSERT(enkf_node->fload);
    return enkf_node->fload(enkf_node->data, filename);
}

/**
   This function loads (internalizes) ECLIPSE results, the ecl_file
   instance with restart data, and the ecl_sum instance with summary
   data must already be loaded by the calling function.

   IFF the enkf_node has registered a filename to load from, that is
   passed to the specific load function, otherwise the run_path is sent
   to the load function.

   If the node does not have a forward_load function, the function just
   returns.
*/
bool enkf_node_forward_load(enkf_node_type *enkf_node,
                            const forward_load_context_type *load_context) {
    bool loadOK;
    FUNC_ASSERT(enkf_node->forward_load);
    {
        if (enkf_node_get_impl_type(enkf_node) == SUMMARY)
            /* Fast path for loading summary data. */
            loadOK =
                enkf_node->forward_load(enkf_node->data, NULL, load_context);
        else {
            char *input_file = enkf_config_node_alloc_infile(
                enkf_node->config,
                forward_load_context_get_load_step(load_context));

            if (input_file != NULL) {
                char *file = util_alloc_filename(
                    forward_load_context_get_run_path(load_context), input_file,
                    NULL);
                loadOK = enkf_node->forward_load(enkf_node->data, file,
                                                 load_context);
                free(file);
            } else
                loadOK = enkf_node->forward_load(enkf_node->data, NULL,
                                                 load_context);

            free(input_file);
        }
    }
    return loadOK;
}

bool enkf_node_forward_init(enkf_node_type *enkf_node, const char *run_path,
                            int iens) {
    char *init_file =
        enkf_config_node_alloc_initfile(enkf_node->config, run_path, iens);
    bool init = enkf_node->initialize(enkf_node->data, iens, init_file, NULL);
    free(init_file);
    return init;
}

bool enkf_node_forward_load_vector(
    enkf_node_type *enkf_node, const forward_load_context_type *load_context,
    const int_vector_type *time_index) {
    bool loadOK;
    FUNC_ASSERT(enkf_node->forward_load_vector);
    loadOK = enkf_node->forward_load_vector(enkf_node->data, NULL, load_context,
                                            time_index);

    return loadOK;
}

static bool enkf_node_store_buffer(enkf_node_type *enkf_node, enkf_fs_type *fs,
                                   int report_step, int iens) {
    FUNC_ASSERT(enkf_node->write_to_buffer);
    {
        bool data_written;
        buffer_type *buffer = buffer_alloc(100);
        const enkf_config_node_type *config_node =
            enkf_node_get_config(enkf_node);
        buffer_fwrite_time_t(buffer, time(NULL));
        data_written =
            enkf_node->write_to_buffer(enkf_node->data, buffer, report_step);
        if (data_written) {
            const char *node_key = enkf_config_node_get_key(config_node);
            enkf_var_type var_type = enkf_config_node_get_var_type(config_node);

            if (enkf_node->vector_storage)
                enkf_fs_fwrite_vector(fs, buffer, node_key, var_type, iens);
            else
                enkf_fs_fwrite_node(fs, buffer, node_key, var_type, report_step,
                                    iens);
        }
        buffer_free(buffer);
        return data_written;
    }
}

bool enkf_node_store_vector(enkf_node_type *enkf_node, enkf_fs_type *fs,
                            int iens) {
    return enkf_node_store_buffer(enkf_node, fs, -1, iens);
}

bool enkf_node_store(enkf_node_type *enkf_node, enkf_fs_type *fs,
                     node_id_type node_id) {
    if (enkf_node->vector_storage)
        return enkf_node_store_vector(enkf_node, fs, node_id.iens);
    else
        return enkf_node_store_buffer(enkf_node, fs, node_id.report_step,
                                      node_id.iens);
}

/**
   This function will load a node from the filesystem if it is
   available; if not it will just return false.

   The state argument can be 'both' - in which case it will first try
   the analyzed, and then subsequently the forecast before giving up
   and returning false. If the function returns true with state ==
   'both' it is no way to determine which version was actually loaded.
*/
bool enkf_node_try_load(enkf_node_type *enkf_node, enkf_fs_type *fs,
                        node_id_type node_id) {
    if (enkf_node_has_data(enkf_node, fs, node_id)) {
        enkf_node_load(enkf_node, fs, node_id);
        return true;
    } else
        return false;
}

static void enkf_node_buffer_load(enkf_node_type *enkf_node, enkf_fs_type *fs,
                                  int report_step, int iens) {
    FUNC_ASSERT(enkf_node->read_from_buffer);
    {
        buffer_type *buffer = buffer_alloc(100);
        const enkf_config_node_type *config_node =
            enkf_node_get_config(enkf_node);
        const char *node_key = enkf_config_node_get_key(config_node);
        enkf_var_type var_type = enkf_config_node_get_var_type(config_node);

        if (enkf_node->vector_storage)
            enkf_fs_fread_vector(fs, buffer, node_key, var_type, iens);
        else
            enkf_fs_fread_node(fs, buffer, node_key, var_type, report_step,
                               iens);

        buffer_fskip_time_t(buffer);

        enkf_node->read_from_buffer(enkf_node->data, buffer, fs, report_step);
        buffer_free(buffer);
    }
}

void enkf_node_load_vector(enkf_node_type *enkf_node, enkf_fs_type *fs,
                           int iens) {
    enkf_node_buffer_load(enkf_node, fs, -1, iens);
}

static void enkf_node_load_container(enkf_node_type *enkf_node,
                                     enkf_fs_type *fs, node_id_type node_id) {
    for (int inode = 0; inode < vector_get_size(enkf_node->container_nodes);
         inode++) {
        enkf_node_type *child_node =
            (enkf_node_type *)vector_iget(enkf_node->container_nodes, inode);
        enkf_node_load(child_node, fs, node_id);
    }
}

void enkf_node_load(enkf_node_type *enkf_node, enkf_fs_type *fs,
                    node_id_type node_id) {
    if (enkf_node_get_impl_type(enkf_node) == CONTAINER)
        enkf_node_load_container(enkf_node, fs, node_id);
    else {
        if (enkf_node->vector_storage)
            enkf_node_load_vector(enkf_node, fs, node_id.iens);
        else
            /* Normal load path */
            enkf_node_buffer_load(enkf_node, fs, node_id.report_step,
                                  node_id.iens);
    }
}

bool enkf_node_try_load_vector(enkf_node_type *enkf_node, enkf_fs_type *fs,
                               int iens) {
    if (enkf_config_node_has_vector(enkf_node->config, fs, iens)) {
        enkf_node_load_vector(enkf_node, fs, iens);
        return true;
    } else
        return false;
}

/**
  In the case of nodes with vector storage this function
  will load the entire vector.
*/
enkf_node_type *enkf_node_load_alloc(const enkf_config_node_type *config_node,
                                     enkf_fs_type *fs, node_id_type node_id) {
    if (enkf_config_node_vector_storage(config_node)) {
        if (enkf_config_node_has_vector(config_node, fs, node_id.iens)) {
            enkf_node_type *node = enkf_node_alloc(config_node);
            enkf_node_load(node, fs, node_id);
            return node;
        } else {
            util_abort("%s: could not load vector:%s from iens:%d\n", __func__,
                       enkf_config_node_get_key(config_node), node_id.iens);
            return NULL;
        }
    } else {
        if (enkf_config_node_has_node(config_node, fs, node_id)) {
            enkf_node_type *node = enkf_node_alloc(config_node);
            enkf_node_load(node, fs, node_id);
            return node;
        } else {
            util_abort("%s: Could not load node: key:%s  iens:%d  report:%d \n",
                       __func__, enkf_config_node_get_key(config_node),
                       node_id.iens, node_id.report_step);
            return NULL;
        }
    }
}

void enkf_node_copy(const enkf_config_node_type *config_node,
                    enkf_fs_type *src_case, enkf_fs_type *target_case,
                    node_id_type src_id, node_id_type target_id) {

    enkf_node_type *enkf_node =
        enkf_node_load_alloc(config_node, src_case, src_id);

    /* Hack to ensure that size is set for the gen_data instances.
     This sneeks low level stuff into a high level scope. BAD. */
    {
        ert_impl_type impl_type = enkf_node_get_impl_type(enkf_node);
        if (impl_type == GEN_DATA) {
            /* Read the size at report_step_from */
            gen_data_type *gen_data =
                (gen_data_type *)enkf_node_value_ptr(enkf_node);
            int size = gen_data_get_size(gen_data);

            /* Enforce the size at report_step_to */
            gen_data_assert_size(gen_data, size, target_id.report_step);
        }
    }

    enkf_node_store(enkf_node, target_case, target_id);
    enkf_node_free(enkf_node);
}

bool enkf_node_has_data(enkf_node_type *enkf_node, enkf_fs_type *fs,
                        node_id_type node_id) {
    if (enkf_node->vector_storage) {
        FUNC_ASSERT(enkf_node->has_data);
        {
            int report_step = node_id.report_step;
            int iens = node_id.iens;

            // Try to load the vector.
            if (enkf_config_node_has_vector(enkf_node->config, fs, iens)) {
                enkf_node_load_vector(enkf_node, fs, iens);

                // The vector is loaded. Check if we have the report_step/state asked for:
                return enkf_node->has_data(enkf_node->data, report_step);
            } else
                return false;
        }
    } else
        return enkf_config_node_has_node(enkf_node->config, fs, node_id);
}

void enkf_node_serialize(enkf_node_type *enkf_node, enkf_fs_type *fs,
                         node_id_type node_id, const ActiveList *active_list,
                         Eigen::MatrixXd &A, int row_offset, int column) {

    FUNC_ASSERT(enkf_node->serialize);
    enkf_node_load(enkf_node, fs, node_id);
    enkf_node->serialize(enkf_node->data, node_id, active_list, A, row_offset,
                         column);
}

void enkf_node_deserialize(enkf_node_type *enkf_node, enkf_fs_type *fs,
                           node_id_type node_id, const ActiveList *active_list,
                           const Eigen::MatrixXd &A, int row_offset,
                           int column) {

    FUNC_ASSERT(enkf_node->deserialize);
    enkf_node->deserialize(enkf_node->data, node_id, active_list, A, row_offset,
                           column);
    enkf_node_store(enkf_node, fs, node_id);
}

/**
   The return value is whether any initialization has actually taken
   place. If the function returns false it is for instance not
   necessary to internalize anything.
*/
bool enkf_node_initialize(enkf_node_type *enkf_node, int iens, rng_type *rng) {
    if (enkf_node_use_forward_init(enkf_node))
        return false; // This node will be initialized by loading results from the forward model.
    else {
        if (enkf_node->initialize != NULL) {
            char *init_file =
                enkf_config_node_alloc_initfile(enkf_node->config, NULL, iens);
            bool init =
                enkf_node->initialize(enkf_node->data, iens, init_file, rng);
            free(init_file);
            return init;
        } else
            return false; /* No init performed */
    }
}

void enkf_node_clear(enkf_node_type *enkf_node) {
    FUNC_ASSERT(enkf_node->clear);
    enkf_node->clear(enkf_node->data);
}

extern "C" void enkf_node_free(enkf_node_type *enkf_node) {
    if (enkf_node->freef != NULL)
        enkf_node->freef(enkf_node->data);
    free(enkf_node->node_key);
    vector_free(enkf_node->container_nodes);
    free(enkf_node);
}

void enkf_node_free__(void *void_node) {
    enkf_node_free((enkf_node_type *)void_node);
}

const char *enkf_node_get_key(const enkf_node_type *enkf_node) {
    return enkf_node->node_key;
}

#undef FUNC_ASSERT

/* Manual inheritance - .... */
static enkf_node_type *
enkf_node_alloc_empty(const enkf_config_node_type *config) {
    const char *node_key = enkf_config_node_get_key(config);
    ert_impl_type impl_type = enkf_config_node_get_impl_type(config);
    enkf_node_type *node = (enkf_node_type *)util_malloc(sizeof *node);
    node->vector_storage = enkf_config_node_vector_storage(config);
    node->config = config;
    node->node_key = util_alloc_string_copy(node_key);
    node->data = NULL;
    node->container_nodes = vector_alloc_new();

    /*
    Start by initializing all function pointers to NULL.
  */
    node->alloc = NULL;
    node->ecl_write = NULL;
    node->forward_load = NULL;
    node->forward_load_vector = NULL;
    node->copy = NULL;
    node->initialize = NULL;
    node->freef = NULL;
    node->free_data = NULL;
    node->user_get = NULL;
    node->user_get_vector = NULL;
    node->fload = NULL;
    node->read_from_buffer = NULL;
    node->write_to_buffer = NULL;
    node->serialize = NULL;
    node->deserialize = NULL;
    node->clear = NULL;
    node->has_data = NULL;

    switch (impl_type) {
    case (CONTAINER):
        node->alloc = container_alloc__;
        node->freef = container_free__;
        break;
    case (GEN_KW):
        node->alloc = gen_kw_alloc__;
        node->ecl_write = gen_kw_ecl_write__;
        node->copy = gen_kw_copy__;
        node->initialize = gen_kw_initialize__;
        node->freef = gen_kw_free__;
        node->user_get = gen_kw_user_get__;
        node->write_to_buffer = gen_kw_write_to_buffer__;
        node->read_from_buffer = gen_kw_read_from_buffer__;
        node->serialize = gen_kw_serialize__;
        node->deserialize = gen_kw_deserialize__;
        node->clear = gen_kw_clear__;
        node->fload = gen_kw_fload__;
        break;
    case (SUMMARY):
        node->forward_load = summary_forward_load__;
        node->forward_load_vector = summary_forward_load_vector__;
        node->alloc = summary_alloc__;
        node->copy = summary_copy__;
        node->freef = summary_free__;
        node->user_get = summary_user_get__;
        node->user_get_vector = summary_user_get_vector__;
        node->read_from_buffer = summary_read_from_buffer__;
        node->write_to_buffer = summary_write_to_buffer__;
        node->serialize = summary_serialize__;
        node->deserialize = summary_deserialize__;
        node->clear = summary_clear__;
        node->has_data = summary_has_data__;
        break;
    case (SURFACE):
        node->initialize = surface_initialize__;
        node->ecl_write = surface_ecl_write__;
        node->alloc = surface_alloc__;
        node->copy = surface_copy__;
        node->freef = surface_free__;
        node->user_get = surface_user_get__;
        node->read_from_buffer = surface_read_from_buffer__;
        node->write_to_buffer = surface_write_to_buffer__;
        node->serialize = surface_serialize__;
        node->deserialize = surface_deserialize__;
        node->clear = surface_clear__;
        node->fload = surface_fload__;
        break;
    case (FIELD):
        node->alloc = field_alloc__;
        node->ecl_write = field_ecl_write__;
        node->copy = field_copy__;
        node->initialize = field_initialize__;
        node->freef = field_free__;
        node->user_get = field_user_get__;
        node->read_from_buffer = field_read_from_buffer__;
        node->write_to_buffer = field_write_to_buffer__;
        node->serialize = field_serialize__;
        node->deserialize = field_deserialize__;

        node->clear = field_clear__;
        node->fload = field_fload__;
        break;
    case (GEN_DATA):
        node->alloc = gen_data_alloc__;
        node->initialize = gen_data_initialize__;
        node->copy = gen_data_copy__;
        node->freef = gen_data_free__;
        node->ecl_write = gen_data_ecl_write__;
        node->forward_load = gen_data_forward_load__;
        node->user_get = gen_data_user_get__;
        node->read_from_buffer = gen_data_read_from_buffer__;
        node->write_to_buffer = gen_data_write_to_buffer__;
        node->serialize = gen_data_serialize__;
        node->deserialize = gen_data_deserialize__;

        node->clear = gen_data_clear__;
        break;
    case (EXT_PARAM):
        node->alloc = ext_param_alloc__;
        node->freef = ext_param_free__;
        node->ecl_write = ext_param_ecl_write__;
        node->write_to_buffer = ext_param_write_to_buffer__;
        node->read_from_buffer = ext_param_read_from_buffer__;
        break;
    default:
        util_abort("%s: implementation type: %d unknown - all hell is loose - "
                   "aborting \n",
                   __func__, impl_type);
    }
    return node;
}

#define CASE_SET(type, func)                                                   \
    case (type):                                                               \
        has_func = (func != NULL);                                             \
        break;
bool enkf_node_has_func(const enkf_node_type *node,
                        node_function_type function_type) {
    bool has_func = false;
    switch (function_type) {
        CASE_SET(alloc_func, node->alloc);
        CASE_SET(ecl_write_func, node->ecl_write);
        CASE_SET(forward_load_func, node->forward_load);
        CASE_SET(copy_func, node->copy);
        CASE_SET(initialize_func, node->initialize);
        CASE_SET(free_func, node->freef);
    default:
        fprintf(stderr,
                "%s: node_function_identifier: %d not recognized - aborting \n",
                __func__, function_type);
    }
    return has_func;
}
#undef CASE_SET

enkf_node_type *enkf_node_alloc(const enkf_config_node_type *config) {
    enkf_node_type *node = enkf_node_alloc_empty(config);
    UTIL_TYPE_ID_INIT(node, ENKF_NODE_TYPE_ID);
    enkf_node_alloc_domain_object(node);
    return node;
}

static void enkf_node_container_add_node(enkf_node_type *node,
                                         const enkf_node_type *child_node,
                                         bool shared) {
    if (shared)
        vector_append_ref(node->container_nodes, child_node);
    else
        vector_append_owned_ref(node->container_nodes, child_node,
                                enkf_node_free__);
}

static enkf_node_type *
enkf_node_alloc_container(const enkf_config_node_type *config,
                          hash_type *node_hash, bool shared) {
    enkf_node_type *container_node = enkf_node_alloc(config);
    {
        for (int i = 0; i < enkf_config_node_container_size(config); i++) {
            const enkf_config_node_type *child_config =
                enkf_config_node_container_iget(config, i);
            enkf_node_type *child_node;

            if (shared)
                child_node = (enkf_node_type *)hash_get(
                    node_hash, enkf_config_node_get_key(child_config));
            else
                child_node = enkf_node_alloc(child_config);

            enkf_node_container_add_node(container_node, child_node, shared);
            container_add_node(
                (container_type *)enkf_node_value_ptr(container_node),
                enkf_node_value_ptr(child_node));
        }
    }
    return container_node;
}

enkf_node_type *
enkf_node_alloc_shared_container(const enkf_config_node_type *config,
                                 hash_type *node_hash) {
    return enkf_node_alloc_container(config, node_hash, true);
}

enkf_node_type *
enkf_node_alloc_private_container(const enkf_config_node_type *config) {
    return enkf_node_alloc_container(config, NULL, false);
}

enkf_node_type *enkf_node_deep_alloc(const enkf_config_node_type *config) {
    if (enkf_config_node_get_impl_type(config) == CONTAINER) {
        enkf_node_type *container =
            enkf_node_alloc_container(config, NULL, false);
        container_assert_size(
            (const container_type *)enkf_node_value_ptr(container));
        return container;
    } else
        return enkf_node_alloc(config);
}

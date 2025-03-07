/*
   Copyright (C) 2013  Equinor ASA, Norway.
   The file 'runpath_list.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <filesystem>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ert/res_util/file_utils.hpp>
#include <ert/util/vector.h>

#include <ert/enkf/runpath_list.hpp>

namespace fs = std::filesystem;

typedef struct runpath_node_struct runpath_node_type;

struct runpath_list_struct {
    pthread_rwlock_t lock;
    vector_type *list;
    /** Format string : Values are in the order: (iens , runpath , basename) */
    char *line_fmt;
    char *export_file;
};

#define RUNPATH_NODE_TYPE_ID 661400541
struct runpath_node_struct {
    UTIL_TYPE_ID_DECLARATION;
    int iens;
    int iter;
    char *runpath;
    char *basename;
};

UTIL_SAFE_CAST_FUNCTION(runpath_node, RUNPATH_NODE_TYPE_ID)
UTIL_SAFE_CAST_FUNCTION_CONST(runpath_node, RUNPATH_NODE_TYPE_ID)

static runpath_node_type *runpath_node_alloc(int iens, int iter,
                                             const char *runpath,
                                             const char *basename) {
    runpath_node_type *node = (runpath_node_type *)util_malloc(sizeof *node);
    UTIL_TYPE_ID_INIT(node, RUNPATH_NODE_TYPE_ID);

    node->iens = iens;
    node->iter = iter;
    node->runpath = util_alloc_string_copy(runpath);
    node->basename = util_alloc_string_copy(basename);

    return node;
}

static void runpath_node_free(runpath_node_type *node) {
    free(node->basename);
    free(node->runpath);
    free(node);
}

static void runpath_node_free__(void *arg) {
    runpath_node_type *node = runpath_node_safe_cast(arg);
    runpath_node_free(node);
}

/**
  The comparison is first based on iteration number and then on iens.
*/
static int runpath_node_cmp(const void *arg1, const void *arg2) {
    const runpath_node_type *node1 = runpath_node_safe_cast_const(arg1);
    const runpath_node_type *node2 = runpath_node_safe_cast_const(arg2);
    {
        if (node1->iter > node2->iter)
            return 1;
        else if (node1->iter < node2->iter)
            return -1;
        else {
            /* Iteration number is the same */
            if (node1->iens > node2->iens)
                return 1;
            else if (node1->iens < node2->iens)
                return -1;
            else
                return 0;
        }
    }
}

static void runpath_node_fprintf(const runpath_node_type *node,
                                 const char *line_fmt, FILE *stream) {
    fprintf(stream, line_fmt, node->iens, node->runpath, node->basename,
            node->iter);
}

runpath_list_type *runpath_list_alloc(const char *export_file) {
    if (export_file == NULL)
        return NULL;

    if (strlen(export_file) == 0)
        return NULL;

    runpath_list_type *list = (runpath_list_type *)util_malloc(sizeof *list);
    list->list = vector_alloc_new();
    list->line_fmt = NULL;
    list->export_file = util_alloc_string_copy(export_file);
    pthread_rwlock_init(&list->lock, NULL);
    return list;
}

void runpath_list_free(runpath_list_type *list) {
    vector_free(list->list);
    free(list->line_fmt);
    free(list->export_file);
    free(list);
}

int runpath_list_size(const runpath_list_type *list) {
    return vector_get_size(list->list);
}

void runpath_list_add(runpath_list_type *list, int iens, int iter,
                      const char *runpath, const char *basename) {
    runpath_node_type *node = runpath_node_alloc(iens, iter, runpath, basename);

    pthread_rwlock_wrlock(&list->lock);
    { vector_append_owned_ref(list->list, node, runpath_node_free__); }
    pthread_rwlock_unlock(&list->lock);
}

void runpath_list_clear(runpath_list_type *list) {
    pthread_rwlock_wrlock(&list->lock);
    { vector_clear(list->list); }
    pthread_rwlock_unlock(&list->lock);
}

void runpath_list_set_line_fmt(runpath_list_type *list, const char *line_fmt) {
    list->line_fmt = util_realloc_string_copy(list->line_fmt, line_fmt);
}

const char *runpath_list_get_line_fmt(const runpath_list_type *list) {
    if (list->line_fmt == NULL)
        return RUNPATH_LIST_DEFAULT_LINE_FMT;
    else
        return list->line_fmt;
}

static const runpath_node_type *
runpath_list_iget_node__(const runpath_list_type *list, int index) {
    return (const runpath_node_type *)vector_iget_const(list->list, index);
}

static const runpath_node_type *runpath_list_iget_node(runpath_list_type *list,
                                                       int index) {
    const runpath_node_type *node;
    {
        pthread_rwlock_rdlock(&list->lock);
        node = runpath_list_iget_node__(list, index);
        pthread_rwlock_unlock(&list->lock);
    }
    return node;
}

int runpath_list_iget_iens(runpath_list_type *list, int index) {
    const runpath_node_type *node = runpath_list_iget_node(list, index);
    return node->iens;
}

int runpath_list_iget_iter(runpath_list_type *list, int index) {
    const runpath_node_type *node = runpath_list_iget_node(list, index);
    return node->iter;
}

char *runpath_list_iget_runpath(runpath_list_type *list, int index) {
    const runpath_node_type *node = runpath_list_iget_node(list, index);
    return node->runpath;
}

char *runpath_list_iget_basename(runpath_list_type *list, int index) {
    const runpath_node_type *node = runpath_list_iget_node(list, index);
    return node->basename;
}

void runpath_list_fprintf(runpath_list_type *list) {
    pthread_rwlock_rdlock(&list->lock);
    {
        auto stream = mkdir_fopen(fs::path(list->export_file), "w");
        const char *line_fmt = runpath_list_get_line_fmt(list);
        int index;
        vector_sort(list->list, runpath_node_cmp);
        for (index = 0; index < vector_get_size(list->list); index++) {
            const runpath_node_type *node =
                runpath_list_iget_node__(list, index);
            runpath_node_fprintf(node, line_fmt, stream);
        }
        fclose(stream);
    }
    pthread_rwlock_unlock(&list->lock);
}

const char *runpath_list_get_export_file(const runpath_list_type *list) {
    return list->export_file;
}

void runpath_list_set_export_file(runpath_list_type *list,
                                  const char *export_file) {
    list->export_file =
        util_realloc_string_copy(list->export_file, export_file);
}

bool runpath_list_load(runpath_list_type *list) {
    FILE *stream = fopen(list->export_file, "r");
    if (!stream)
        return false;

    {
        vector_type *tmp_nodes = vector_alloc_new();
        bool read_ok = true;
        while (true) {
            char basename[256], runpath[256];
            int iens, iter;

            int read_count = fscanf(stream, "%d %255s %255s %d", &iens, runpath,
                                    basename, &iter);
            if (read_count == 4)
                vector_append_ref(
                    tmp_nodes,
                    runpath_node_alloc(iens, iter, runpath, basename));
            else {
                if (read_count != EOF)
                    read_ok = false;

                break;
            }
        }
        fclose(stream);

        if (read_ok) {
            pthread_rwlock_wrlock(&list->lock);
            for (int i = 0; i < vector_get_size(tmp_nodes); i++) {
                runpath_node_type *node =
                    (runpath_node_type *)vector_iget(tmp_nodes, i);
                vector_append_owned_ref(list->list, node, runpath_node_free__);
            }
            pthread_rwlock_unlock(&list->lock);
        } else {
            for (int i = 0; i < vector_get_size(tmp_nodes); i++) {
                runpath_node_type *node =
                    (runpath_node_type *)vector_iget(tmp_nodes, i);
                runpath_node_free(node);
            }
        }

        vector_free(tmp_nodes);
        return read_ok;
    }
}

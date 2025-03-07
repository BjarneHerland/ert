/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'rms_file.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_RMS_FILE_H
#define ERT_RMS_FILE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <ert/rms/rms_tag.hpp>

typedef struct rms_file_struct rms_file_type;

void rms_file_fclose(rms_file_type *);
FILE *rms_file_fopen_r(rms_file_type *rms_file);
FILE *rms_file_fopen_w(rms_file_type *rms_file);
void rms_file_set_filename(rms_file_type *, const char *, bool);
rms_file_type *rms_file_alloc(const char *, bool);
void rms_file_free(rms_file_type *);
void rms_file_free_data(rms_file_type *);
rms_tag_type *rms_file_get_dim_tag_ref(const rms_file_type *);
rms_tag_type *rms_file_get_tag_ref(const rms_file_type *, const char *,
                                   const char *, const char *, bool);
rms_tag_type *rms_file_fread_alloc_tag(rms_file_type *, const char *,
                                       const char *, const char *);
rms_tagkey_type *rms_file_fread_alloc_data_tagkey(rms_file_type *, const char *,
                                                  const char *, const char *);
void rms_file_complete_fwrite(const rms_file_type *);
void rms_file_init_fwrite(const rms_file_type *, const char *);
void rms_file_get_dims(const rms_file_type *, int *);
FILE *rms_file_get_FILE(const rms_file_type *);
bool rms_file_is_roff(FILE *);

#endif

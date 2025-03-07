/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'rms_tag.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_RMS_TAG_H
#define ERT_RMS_TAG_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <ert/util/hash.hpp>

#include <ert/rms/rms_tagkey.hpp>

typedef struct rms_tag_struct rms_tag_type;

const char *rms_tag_get_namekey_name(const rms_tag_type *);
const char *rms_tag_get_name(const rms_tag_type *);
void rms_tag_free(rms_tag_type *);
void rms_tag_free__(void *arg);
rms_tag_type *rms_tag_fread_alloc(FILE *, hash_type *, bool, bool *);
bool rms_tag_name_eq(const rms_tag_type *, const char *, const char *,
                     const char *);
rms_tagkey_type *rms_tag_get_key(const rms_tag_type *, const char *);
void rms_tag_fwrite_filedata(const char *, FILE *stream);
void rms_tag_fwrite_eof(FILE *stream);
void rms_tag_fwrite(const rms_tag_type *, FILE *);
rms_tag_type *rms_tag_alloc_dimensions(int, int, int);
void rms_tag_fwrite_dimensions(int, int, int, FILE *);
void rms_tag_fwrite_parameter(const char *, const rms_tagkey_type *, FILE *);
#endif

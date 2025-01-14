/*
   Copyright (C) 2012  Equinor ASA, Norway.

   The file 'config_error.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_CONFIG_ERROR_H
#define ERT_CONFIG_ERROR_H

#include <stdio.h>

typedef struct config_error_struct config_error_type;

config_error_type *config_error_alloc();
config_error_type *config_error_alloc_copy(const config_error_type *src_error);
extern "C" void config_error_free(config_error_type *error);
extern "C" const char *config_error_iget(const config_error_type *error,
                                         int index);
void config_error_add(config_error_type *error, char *new_error);
extern "C" int config_error_count(const config_error_type *error);
void config_error_fprintf(const config_error_type *error, bool add_count,
                          FILE *stream);
bool config_error_equal(const config_error_type *error1,
                        const config_error_type *error2);

#endif

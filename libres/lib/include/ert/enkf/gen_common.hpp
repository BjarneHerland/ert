/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'gen_common.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_GEN_COMMON_H
#define ERT_GEN_COMMON_H

#include <stdio.h>
#include <stdlib.h>

#include <ert/ecl/ecl_type.h>
#include <ert/enkf/gen_data_config.hpp>

void *gen_common_fscanf_alloc(const char *, ecl_data_type, int *);
void *gen_common_fread_alloc(const char *, ecl_data_type, int *);
void *gen_common_fload_alloc(const char *, gen_data_file_format_type,
                             ecl_data_type, ecl_type_enum *, int *);

#endif

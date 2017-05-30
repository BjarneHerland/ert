/*
   Copyright (C) 2017  Statoil ASA, Norway.

   The file 'enkf_config.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_ENKF_CONFIG_H
#define ERT_ENKF_CONFIG_H

#include <ert/enkf/site_config.h>
#include <ert/enkf/rng_config.h>

typedef struct enkf_config_struct enkf_config_type;

enkf_config_type * enkf_config_alloc_load(const char *);
void enkf_config_free(enkf_config_type *);

site_config_type * enkf_config_get_site_config(const enkf_config_type *);
rng_config_type  * enkf_config_get_rng_config(const enkf_config_type *);

const char * enkf_config_get_user_config_file(const enkf_config_type *);

#endif

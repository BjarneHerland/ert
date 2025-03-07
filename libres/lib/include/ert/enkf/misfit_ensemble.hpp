/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'misfit_table.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_MISFIT_ENSEMBLE_H
#define ERT_MISFIT_ENSEMBLE_H

#include <stdbool.h>

#include <ert/enkf/enkf_fs.hpp>
#include <ert/enkf/enkf_obs.hpp>
#include <ert/enkf/ensemble_config.hpp>
#include <ert/enkf/misfit_member.hpp>

#include <ert/enkf/misfit_ensemble_typedef.hpp>

void misfit_ensemble_fread(misfit_ensemble_type *misfit_ensemble, FILE *stream);
void misfit_ensemble_clear(misfit_ensemble_type *table);
misfit_ensemble_type *misfit_ensemble_alloc();
void misfit_ensemble_free(misfit_ensemble_type *table);
void misfit_ensemble_fwrite(const misfit_ensemble_type *misfit_ensemble,
                            FILE *stream);
bool misfit_ensemble_initialized(const misfit_ensemble_type *misfit_ensemble);

void misfit_ensemble_initialize(misfit_ensemble_type *misfit_ensemble,
                                const ensemble_config_type *ensemble_config,
                                const enkf_obs_type *enkf_obs, enkf_fs_type *fs,
                                int ens_size, int history_length,
                                bool force_init);

void misfit_ensemble_set_ens_size(misfit_ensemble_type *misfit_ensemble,
                                  int ens_size);
int misfit_ensemble_get_ens_size(const misfit_ensemble_type *misfit_ensemble);

misfit_member_type *
misfit_ensemble_iget_member(const misfit_ensemble_type *table, int iens);

#endif

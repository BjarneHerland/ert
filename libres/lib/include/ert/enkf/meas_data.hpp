/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'meas_data.h' is part of ERT - Ensemble based Reservoir Tool.

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

#ifndef ERT_MEAS_DATA_H
#define ERT_MEAS_DATA_H

#include <Eigen/Dense>
#include <stdbool.h>
#include <vector>

#include <ert/util/bool_vector.h>
#include <ert/util/hash.h>
#include <ert/util/type_macros.h>

#include <ert/tooling.hpp>

typedef struct meas_data_struct meas_data_type;
typedef struct meas_block_struct meas_block_type;

UTIL_IS_INSTANCE_HEADER(meas_data);
UTIL_SAFE_CAST_HEADER(meas_block);
meas_block_type *meas_block_alloc(const char *obs_key,
                                  const std::vector<bool> &ens_mask,
                                  int obs_size);
extern "C" int meas_block_get_total_ens_size(const meas_block_type *meas_block);
extern "C" PY_USED int
meas_block_get_active_ens_size(const meas_block_type *meas_block);
extern "C" bool meas_block_iens_active(const meas_block_type *meas_block,
                                       int iens);
extern "C" void meas_block_iset(meas_block_type *meas_block, int iens, int iobs,
                                double value);
extern "C" double meas_block_iget(const meas_block_type *meas_block, int iens,
                                  int iobs);
extern "C" double meas_block_iget_ens_mean(meas_block_type *meas_block,
                                           int iobs);
extern "C" double meas_block_iget_ens_std(meas_block_type *meas_block,
                                          int iobs);
void meas_block_deactivate(meas_block_type *meas_block, int iobs);
bool meas_block_iget_active(const meas_block_type *meas_block, int iobs);
extern "C" void meas_block_free(meas_block_type *meas_block);

extern "C" bool meas_data_has_block(const meas_data_type *matrix,
                                    const char *lookup_key);

extern "C" meas_block_type *meas_data_get_block(const meas_data_type *matrix,
                                                const char *lookup_key);
meas_data_type *meas_data_alloc(const std::vector<bool> &ens_mask);

extern "C" void meas_data_free(meas_data_type *);
Eigen::MatrixXd meas_data_makeS(const meas_data_type *matrix);
extern "C" int meas_data_get_active_obs_size(const meas_data_type *matrix);
extern "C" int meas_data_get_total_ens_size(const meas_data_type *matrix);
extern "C" int meas_data_get_active_ens_size(const meas_data_type *meas_data);
extern "C" meas_block_type *meas_data_add_block(meas_data_type *matrix,
                                                const char *obs_key,
                                                int report_step, int obs_size);
extern "C" int meas_data_get_num_blocks(const meas_data_type *meas_block);
extern "C" meas_block_type *meas_data_iget_block(const meas_data_type *matrix,
                                                 int block_mnr);

const meas_block_type *meas_data_iget_block_const(const meas_data_type *matrix,
                                                  int block_nr);
extern "C" int meas_block_get_total_obs_size(const meas_block_type *meas_block);

#endif

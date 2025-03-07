/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'fs_types.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/enkf/fs_types.hpp>

/**
  @brief returns whether fs type is valid.

  The driver type DRIVER_STATIC has been removed completely as of
  December 2015 and therefore not valid. There will still be many mount map files with
  this enum value around on disk. This function is a minor convenience
  to handle that.

  The driver type DRIVER_DYNAMIC_ANALYZED was removed ~april 2016, and is not valid.
*/
bool fs_types_valid(fs_driver_enum driver_type) {
    if ((driver_type == DRIVER_STATIC) ||
        (driver_type == DRIVER_DYNAMIC_ANALYZED))
        return false;
    else
        return true;
}

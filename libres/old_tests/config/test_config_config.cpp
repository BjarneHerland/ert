/*
   Copyright (C) 2013  Equinor ASA, Norway.

   The file 'config_config.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <stdlib.h>

#include <ert/config/config_parser.hpp>

int main(int argc, char **argv) {
    config_parser_type *config = config_alloc();
    config_add_schema_item(config, "KEYWORD", false);
    config_free(config);
    exit(0);
}

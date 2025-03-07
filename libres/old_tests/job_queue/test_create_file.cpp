/*
   Copyright (C) 2012  Equinor ASA, Norway.

   The file 'create_file.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    char *filename = argv[1];
    int value = atoi(argv[2]);
    FILE *stream = fopen(filename, "w");
    fprintf(stream, "%d\n", value);
    fclose(stream);
}

/*
   Copyright (C) 2015  Equinor ASA, Norway.

   The file 'ert_util_subst_list.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <ert/res_util/subst_list.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

void test_create() {
    subst_list_type *subst_list = subst_list_alloc(NULL);
    test_assert_true(subst_list_is_instance(subst_list));
    subst_list_free(subst_list);
}

void test_filter_file1() {
    subst_list_type *subst_list = subst_list_alloc(NULL);
    ecl::util::TestArea ta("filter_:file");
    {
        FILE *stream = util_fopen("template", "w");
        fprintf(stream, "<KEY1>\n<KEY2>\n<KEY3>\n<KEY4>\n");
        fclose(stream);
    }
    subst_list_append_copy(subst_list, "<KEY1>", "Value1", NULL);
    subst_list_append_copy(subst_list, "<KEY2>", "Value2", NULL);
    subst_list_append_copy(subst_list, "<KEY3>", "Value3", NULL);
    subst_list_append_copy(subst_list, "<KEY4>", "Value4", NULL);

    subst_list_filter_file(subst_list, "template", "target");

    {
        FILE *stream = util_fopen("target", "r");
        char s1[128], s2[128], s3[128], s4[128];

        test_assert_int_equal(4, fscanf(stream, "%s %s %s %s", s1, s2, s3, s4));
        fclose(stream);

        test_assert_string_equal(s1, "Value1");
        test_assert_string_equal(s2, "Value2");
        test_assert_string_equal(s3, "Value3");
        test_assert_string_equal(s4, "Value4");
    }
    subst_list_free(subst_list);
}

void test_filter_file2() {
    subst_list_type *subst_list = subst_list_alloc(NULL);
    ecl::util::TestArea ta("filetr2");
    {
        FILE *stream = util_fopen("template", "w");
        fprintf(stream, "MAGIC_PRINT  magic-list.txt  <ERTCASE>  __MAGIC__");
        fclose(stream);
    }

    subst_list_append_copy(subst_list, "__MAGIC__", "MagicAllTheWayToWorkFlow",
                           NULL);
    subst_list_append_copy(subst_list, "<CASE>", "SUPERcase", NULL);
    subst_list_append_copy(subst_list, "<ERTCASE>", "default", NULL);
    subst_list_filter_file(subst_list, "template", "target");

    {
        char *target_string = util_fread_alloc_file_content("target", NULL);
        test_assert_string_equal(
            target_string,
            "MAGIC_PRINT  magic-list.txt  default  MagicAllTheWayToWorkFlow");
        free(target_string);
    }
    subst_list_free(subst_list);
}

int main(int argc, char **argv) {
    test_create();
    test_filter_file1();
    test_filter_file2();
}

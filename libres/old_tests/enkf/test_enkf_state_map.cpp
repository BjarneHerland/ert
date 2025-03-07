/*
   Copyright (C) 2013  Equinor ASA, Norway.

   The file 'enkf_state_map.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <vector>

#include <stdlib.h>

#include <ert/util/bool_vector.h>
#include <ert/util/test_util.h>
#include <ert/util/test_work_area.hpp>

#include <ert/enkf/state_map.hpp>

void create_test() {
    state_map_type *state_map = state_map_alloc();
    test_assert_true(state_map_is_instance(state_map));
    test_assert_int_equal(0, state_map_get_size(state_map));
    test_assert_false(state_map_is_readonly(state_map));
    state_map_free(state_map);
}

void get_test() {
    state_map_type *state_map = state_map_alloc();
    test_assert_int_equal(STATE_UNDEFINED, state_map_iget(state_map, 0));
    test_assert_int_equal(STATE_UNDEFINED, state_map_iget(state_map, 100));
    state_map_free(state_map);
}

void set_test() {
    state_map_type *state_map = state_map_alloc();
    state_map_iset(state_map, 0, STATE_INITIALIZED);
    test_assert_int_equal(STATE_INITIALIZED, state_map_iget(state_map, 0));

    state_map_iset(state_map, 100, STATE_INITIALIZED);
    test_assert_int_equal(STATE_INITIALIZED, state_map_iget(state_map, 100));

    test_assert_int_equal(STATE_UNDEFINED, state_map_iget(state_map, 50));
    test_assert_int_equal(101, state_map_get_size(state_map));
    state_map_free(state_map);
}

void load_empty_test() {
    state_map_type *state_map = state_map_fread_alloc("File/does/not/exists");
    test_assert_true(state_map_is_instance(state_map));
    test_assert_int_equal(0, state_map_get_size(state_map));
    state_map_free(state_map);
}

void test_equal() {
    state_map_type *state_map1 = state_map_alloc();
    state_map_type *state_map2 = state_map_alloc();

    test_assert_true(state_map_equal(state_map1, state_map2));
    for (int i = 0; i < 25; i++) {
        state_map_iset(state_map1, i, STATE_INITIALIZED);
        state_map_iset(state_map2, i, STATE_INITIALIZED);
    }
    test_assert_true(state_map_equal(state_map1, state_map2));

    state_map_iset(state_map2, 15, STATE_HAS_DATA);
    test_assert_false(state_map_equal(state_map1, state_map2));
    state_map_iset(state_map2, 15, STATE_LOAD_FAILURE);
    state_map_iset(state_map2, 15, STATE_INITIALIZED);
    test_assert_true(state_map_equal(state_map1, state_map2));

    state_map_iset(state_map2, 150, STATE_INITIALIZED);
    test_assert_false(state_map_equal(state_map1, state_map2));
}

void test_copy() {
    state_map_type *state_map = state_map_alloc();
    state_map_iset(state_map, 0, STATE_INITIALIZED);
    state_map_iset(state_map, 100, STATE_INITIALIZED);
    {
        state_map_type *copy = state_map_alloc_copy(state_map);
        test_assert_true(state_map_equal(copy, state_map));

        state_map_iset(state_map, 10, STATE_INITIALIZED);
        test_assert_false(state_map_equal(copy, state_map));

        state_map_free(copy);
    }
    state_map_free(state_map);
}

void test_io() {
    ecl::util::TestArea ta("state_map_io");
    {
        state_map_type *state_map = state_map_alloc();
        state_map_type *copy1, *copy2;
        state_map_iset(state_map, 0, STATE_INITIALIZED);
        state_map_iset(state_map, 100, STATE_INITIALIZED);
        state_map_fwrite(state_map, "map");

        copy1 = state_map_fread_alloc("map");
        test_assert_true(state_map_equal(state_map, copy1));

        copy2 = state_map_alloc();
        test_assert_true(state_map_fread(copy2, "map"));
        test_assert_true(state_map_equal(state_map, copy2));

        state_map_iset(copy2, 67, STATE_INITIALIZED);
        test_assert_false(state_map_equal(state_map, copy2));

        state_map_fread(copy2, "map");
        test_assert_true(state_map_equal(state_map, copy2));

        test_assert_false(state_map_fread(copy2, "DoesNotExist"));
        test_assert_int_equal(0, state_map_get_size(copy2));
    }
}

void test_update_undefined() {
    state_map_type *map = state_map_alloc();

    state_map_iset(map, 10, STATE_INITIALIZED);
    test_assert_int_equal(STATE_UNDEFINED, state_map_iget(map, 5));
    test_assert_int_equal(STATE_INITIALIZED, state_map_iget(map, 10));

    state_map_update_undefined(map, 5, STATE_INITIALIZED);
    test_assert_int_equal(STATE_INITIALIZED, state_map_iget(map, 5));

    state_map_update_undefined(map, 10, STATE_INITIALIZED);
    test_assert_int_equal(STATE_INITIALIZED, state_map_iget(map, 10));

    state_map_free(map);
}

void test_update_matching() {
    state_map_type *map = state_map_alloc();

    state_map_iset(map, 10, STATE_INITIALIZED);
    state_map_iset(map, 3, STATE_PARENT_FAILURE);
    test_assert_int_equal(STATE_UNDEFINED, state_map_iget(map, 5));
    test_assert_int_equal(STATE_INITIALIZED, state_map_iget(map, 10));

    state_map_update_matching(map, 5, STATE_UNDEFINED | STATE_LOAD_FAILURE,
                              STATE_INITIALIZED);
    state_map_update_matching(map, 10, STATE_UNDEFINED | STATE_LOAD_FAILURE,
                              STATE_INITIALIZED);
    state_map_update_matching(map, 3, STATE_UNDEFINED | STATE_LOAD_FAILURE,
                              STATE_INITIALIZED);

    test_assert_int_equal(STATE_INITIALIZED, state_map_iget(map, 5));
    test_assert_int_equal(STATE_INITIALIZED, state_map_iget(map, 10));
    test_assert_int_equal(STATE_PARENT_FAILURE, state_map_iget(map, 3));

    state_map_update_undefined(map, 10, STATE_INITIALIZED);
    test_assert_int_equal(STATE_INITIALIZED, state_map_iget(map, 10));

    state_map_free(map);
}

void test_select_matching() {
    state_map_type *map = state_map_alloc();

    state_map_iset(map, 10, STATE_INITIALIZED);
    state_map_iset(map, 10, STATE_HAS_DATA);
    state_map_iset(map, 20, STATE_INITIALIZED);
    std::vector<bool> mask = state_map_select_matching(
        map, STATE_HAS_DATA | STATE_INITIALIZED, true);
    test_assert_int_equal(mask.size(), 21);
    test_assert_true(mask[10]);
    test_assert_true(mask[20]);

    mask = state_map_select_matching(map, STATE_HAS_DATA, true);

    for (size_t i; i < mask.size(); i++) {
        if (i == 10)
            test_assert_true(mask[i]);
        else {
            test_assert_false(mask[0]);
        }
    }

    state_map_iset(map, 50, STATE_INITIALIZED);
    mask = state_map_select_matching(map, STATE_HAS_DATA | STATE_INITIALIZED,
                                     true);
    test_assert_int_equal(mask.size(), 51);

    state_map_free(map);
}

void test_deselect_matching() {
    state_map_type *map = state_map_alloc();

    state_map_iset(map, 10, STATE_HAS_DATA);
    state_map_iset(map, 20, STATE_INITIALIZED);
    std::vector<bool> mask = state_map_select_matching(
        map, STATE_HAS_DATA | STATE_INITIALIZED, false);

    test_assert_int_equal(state_map_get_size(map), mask.size());

    for (int i = 0; i < mask.size(); i++) {
        if ((i == 10) | (i == 20))
            test_assert_false(mask[i]);
        else {
            test_assert_true(mask[i]);
        }
    }

    state_map_free(map);
}

void test_count_matching() {
    state_map_type *map1 = state_map_alloc();
    state_map_iset(map1, 10, STATE_INITIALIZED);

    state_map_iset(map1, 15, STATE_INITIALIZED);
    state_map_iset(map1, 15, STATE_HAS_DATA);

    state_map_iset(map1, 16, STATE_INITIALIZED);
    state_map_iset(map1, 16, STATE_HAS_DATA);
    state_map_iset(map1, 16, STATE_LOAD_FAILURE);

    test_assert_int_equal(1, state_map_count_matching(map1, STATE_HAS_DATA));
    test_assert_int_equal(
        2, state_map_count_matching(map1, STATE_HAS_DATA | STATE_LOAD_FAILURE));
    test_assert_int_equal(
        3, state_map_count_matching(map1, STATE_HAS_DATA | STATE_LOAD_FAILURE |
                                              STATE_INITIALIZED));

    state_map_free(map1);
}

// Probably means that the target should be explicitly set to
// undefined before workflows which automatically change case.
void test_transitions() {

    test_assert_false(
        state_map_legal_transition(STATE_UNDEFINED, STATE_UNDEFINED));
    test_assert_true(
        state_map_legal_transition(STATE_UNDEFINED, STATE_INITIALIZED));
    test_assert_false(
        state_map_legal_transition(STATE_UNDEFINED, STATE_HAS_DATA));
    test_assert_false(
        state_map_legal_transition(STATE_UNDEFINED, STATE_LOAD_FAILURE));
    test_assert_true(
        state_map_legal_transition(STATE_UNDEFINED, STATE_PARENT_FAILURE));

    test_assert_false(
        state_map_legal_transition(STATE_INITIALIZED, STATE_UNDEFINED));
    test_assert_true(
        state_map_legal_transition(STATE_INITIALIZED, STATE_INITIALIZED));
    test_assert_true(
        state_map_legal_transition(STATE_INITIALIZED, STATE_HAS_DATA));
    test_assert_true(
        state_map_legal_transition(STATE_INITIALIZED, STATE_LOAD_FAILURE));
    test_assert_true(state_map_legal_transition(
        STATE_INITIALIZED,
        STATE_PARENT_FAILURE)); // Should maybe false - if the commenta baove is taken into account.

    test_assert_false(
        state_map_legal_transition(STATE_HAS_DATA, STATE_UNDEFINED));
    test_assert_true(
        state_map_legal_transition(STATE_HAS_DATA, STATE_INITIALIZED));
    test_assert_true(
        state_map_legal_transition(STATE_HAS_DATA, STATE_HAS_DATA));
    test_assert_true(
        state_map_legal_transition(STATE_HAS_DATA, STATE_LOAD_FAILURE));
    test_assert_true(state_map_legal_transition(STATE_HAS_DATA,
                                                STATE_PARENT_FAILURE)); // Rerun

    test_assert_false(
        state_map_legal_transition(STATE_LOAD_FAILURE, STATE_UNDEFINED));
    test_assert_true(
        state_map_legal_transition(STATE_LOAD_FAILURE, STATE_INITIALIZED));
    test_assert_true(
        state_map_legal_transition(STATE_LOAD_FAILURE, STATE_HAS_DATA));
    test_assert_true(
        state_map_legal_transition(STATE_LOAD_FAILURE, STATE_LOAD_FAILURE));
    test_assert_false(
        state_map_legal_transition(STATE_LOAD_FAILURE, STATE_PARENT_FAILURE));

    test_assert_false(
        state_map_legal_transition(STATE_PARENT_FAILURE, STATE_UNDEFINED));
    test_assert_true(
        state_map_legal_transition(STATE_PARENT_FAILURE, STATE_INITIALIZED));
    test_assert_false(
        state_map_legal_transition(STATE_PARENT_FAILURE, STATE_HAS_DATA));
    test_assert_false(
        state_map_legal_transition(STATE_PARENT_FAILURE, STATE_LOAD_FAILURE));
    test_assert_true(
        state_map_legal_transition(STATE_PARENT_FAILURE, STATE_PARENT_FAILURE));
}

void test_readonly() {
    {
        state_map_type *map1 =
            state_map_fread_alloc_readonly("FileDoesNotExist");

        test_assert_true(state_map_is_instance(map1));
        test_assert_int_equal(0, state_map_get_size(map1));
        test_assert_true(state_map_is_readonly(map1));
        state_map_free(map1);
    }
    {
        ecl::util::TestArea ta("ro");
        state_map_type *map1 = state_map_alloc();

        state_map_iset(map1, 5, STATE_INITIALIZED);
        state_map_iset(map1, 9, STATE_INITIALIZED);

        state_map_fwrite(map1, "map1");
        {
            state_map_type *map2 = state_map_fread_alloc_readonly("map1");

            test_assert_true(state_map_equal(map1, map2));
            state_map_free(map2);
        }
        state_map_free(map1);
    }
}

int main(int argc, char **argv) {
    create_test();
    get_test();
    set_test();
    load_empty_test();
    test_equal();
    test_copy();
    test_io();
    test_update_undefined();
    test_select_matching();
    test_count_matching();
    test_transitions();
    test_readonly();
    exit(0);
}

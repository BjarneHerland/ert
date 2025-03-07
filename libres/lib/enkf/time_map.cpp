/*
   Copyright (C) 2011  Equinor ASA, Norway.
   The file 'time_map.c' is part of ERT - Ensemble based Reservoir Tool.

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
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fmt/format.h>
#include <iomanip>

#include <pthread.h>
#include <stdlib.h>

#include <ert/res_util/file_utils.hpp>
#include <ert/util/time_t_vector.h>
#include <ert/util/util.h>

#include <ert/ecl/ecl_sum.h>

#include <ert/enkf/time_map.hpp>
#include <ert/logging.hpp>

namespace fs = std::filesystem;
static auto logger = ert::get_logger("enkf");

#define DEFAULT_TIME -1

static time_t time_map_iget__(const time_map_type *map, int step);
static void time_map_update_abort(time_map_type *map, int step, time_t time);
static void time_map_summary_log_mismatch(time_map_type *map,
                                          const ecl_sum_type *ecl_sum);

#define TIME_MAP_TYPE_ID 7751432
struct time_map_struct {
    UTIL_TYPE_ID_DECLARATION;
    time_t_vector_type *map;
    pthread_rwlock_t rw_lock;
    bool modified;
    bool read_only;
    const ecl_sum_type *refcase;
};

UTIL_SAFE_CAST_FUNCTION(time_map, TIME_MAP_TYPE_ID)
UTIL_IS_INSTANCE_FUNCTION(time_map, TIME_MAP_TYPE_ID)

time_map_type *time_map_alloc() {
    time_map_type *map = (time_map_type *)util_malloc(sizeof *map);
    UTIL_TYPE_ID_INIT(map, TIME_MAP_TYPE_ID);

    map->map = time_t_vector_alloc(0, DEFAULT_TIME);
    map->modified = false;
    map->read_only = false;
    map->refcase = NULL;
    pthread_rwlock_init(&map->rw_lock, NULL);
    return map;
}

/**
   The refcase will only be attached if it is consistent with the
   current time map; we will accept attaching a refcase which is
   shorter than the current case.
*/
bool time_map_attach_refcase(time_map_type *time_map,
                             const ecl_sum_type *refcase) {
    bool attach_ok = true;
    pthread_rwlock_rdlock(&time_map->rw_lock);

    {
        int step;
        int max_step = std::min(time_map_get_size(time_map),
                                ecl_sum_get_last_report_step(refcase) + 1);

        for (step = 0; step < max_step; step++) {
            time_t current_time = time_map_iget__(time_map, step);
            time_t sim_time = ecl_sum_get_report_time(refcase, step);

            if (current_time != sim_time) {
                /*
          The treatment of report step 0 has been fraught with uncertainty.
          Report step 0 is not really valid, and previously both the time_map
          and the ecl_sum instance have returned -1 as an indication of
          undefined value.

          As part of the refactoring when the unsmry_loader was added to the
          ecl_sum implementation ecl_sum will return the start date for report
          step 0, then this test will fail for existing time_maps created prior
          to this change of behaviour. We therefore special case the step 0 here.

          July 2018.
        */
                if (step == 0)
                    continue;

                attach_ok = false;
                break;
            }
        }

        if (attach_ok)
            time_map->refcase = refcase;
    }
    pthread_rwlock_unlock(&time_map->rw_lock);

    return attach_ok;
}

bool time_map_has_refcase(const time_map_type *time_map) {
    if (time_map->refcase)
        return true;
    else
        return false;
}

bool time_map_fscanf(time_map_type *map, const char *filename) {
    bool fscanf_ok = true;
    if (util_is_file(filename)) {
        time_t_vector_type *time_vector = time_t_vector_alloc(0, 0);

        {
            FILE *stream = util_fopen(filename, "r");
            time_t last_date = 0;
            while (true) {
                char date_string[128];
                if (fscanf(stream, "%s", date_string) == 1) {
                    time_t date;
                    bool date_parsed_ok =
                        util_sscanf_isodate(date_string, &date);
                    if (!date_parsed_ok) {
                        date_parsed_ok =
                            util_sscanf_date_utc(date_string, &date);
                        fprintf(stderr,
                                "** Deprecation warning: The date format as in "
                                "\'%s\' is deprecated, and its support will be "
                                "removed in a future release. Please use ISO "
                                "date format YYYY-MM-DD.\n",
                                date_string);
                    }
                    if (date_parsed_ok) {
                        if (date > last_date)
                            time_t_vector_append(time_vector, date);
                        else {
                            fprintf(stderr,
                                    "** ERROR: The dates in %s must be in "
                                    "strictly increasing order\n",
                                    filename);
                            fscanf_ok = false;
                            break;
                        }
                    } else {
                        fprintf(stderr,
                                "** ERROR: The string \'%s\' was not correctly "
                                "parsed as a date. "
                                "Please use ISO date format YYYY-MM-DD.\n",
                                date_string);
                        fscanf_ok = false;
                        break;
                    }
                    last_date = date;
                } else
                    break;
            }
            fclose(stream);

            if (fscanf_ok) {
                int i;
                time_map_clear(map);
                for (i = 0; i < time_t_vector_size(time_vector); i++)
                    time_map_update(map, i, time_t_vector_iget(time_vector, i));
            }
        }
        time_t_vector_free(time_vector);
    } else
        fscanf_ok = false;

    return fscanf_ok;
}

bool time_map_equal(const time_map_type *map1, const time_map_type *map2) {
    return time_t_vector_equal(map1->map, map2->map);
}

void time_map_free(time_map_type *map) {
    time_t_vector_free(map->map);
    free(map);
}

bool time_map_is_readonly(const time_map_type *tm) { return tm->read_only; }

/**
   Must hold the write lock. When a refcase is supplied we gurantee
   that all values written into the map agree with the refcase
   values. However the time map is not preinitialized with the refcase
   values.
*/
static bool time_map_update__(time_map_type *map, int step,
                              time_t update_time) {
    bool updateOK = true;
    time_t current_time = time_t_vector_safe_iget(map->map, step);

    if (current_time == DEFAULT_TIME) {
        if (map->refcase) {
            if (step <= ecl_sum_get_last_report_step(map->refcase)) {
                time_t ref_time = ecl_sum_get_report_time(map->refcase, step);

                if (ref_time != update_time) {
                    updateOK = false;
                    logger->error("Tried to load data where report step/data "
                                  "is incompatible with refcase - ignored");
                }
            }
        }
    } else if (current_time != update_time)
        updateOK = false;

    if (updateOK) {
        map->modified = true;
        time_t_vector_iset(map->map, step, update_time);
    }

    return updateOK;
}

static bool time_map_summary_update__(time_map_type *map,
                                      const ecl_sum_type *ecl_sum) {
    bool updateOK = true;
    int first_step = ecl_sum_get_first_report_step(ecl_sum);
    int last_step = ecl_sum_get_last_report_step(ecl_sum);
    int step;

    for (step = first_step; step <= last_step; step++) {
        if (ecl_sum_has_report_step(ecl_sum, step)) {
            time_t sim_time = ecl_sum_get_report_time(ecl_sum, step);

            updateOK = (updateOK && time_map_update__(map, step, sim_time));
        }
    }

    updateOK = (updateOK &&
                time_map_update__(map, 0, ecl_sum_get_start_time(ecl_sum)));
    return updateOK;
}

static time_t time_map_iget__(const time_map_type *map, int step) {
    return time_t_vector_safe_iget(map->map, step);
}

double time_map_iget_sim_days(time_map_type *map, int step) {
    double days;

    pthread_rwlock_rdlock(&map->rw_lock);
    {
        time_t start_time = time_map_iget__(map, 0);
        time_t sim_time = time_map_iget__(map, step);

        if (sim_time >= start_time)
            days = 1.0 * (sim_time - start_time) / (3600 * 24);
        else
            days = -1;
    }
    pthread_rwlock_unlock(&map->rw_lock);

    return days;
}

time_t time_map_iget(time_map_type *map, int step) {
    time_t t;

    pthread_rwlock_rdlock(&map->rw_lock);
    t = time_map_iget__(map, step);
    pthread_rwlock_unlock(&map->rw_lock);

    return t;
}

static void time_map_assert_writable(const time_map_type *map) {
    if (map->read_only)
        util_abort("%s: attempt to modify read-only time-map. \n", __func__);
}

/**
   Observe that the locking is opposite of the function name; i.e.
   the time_map_fwrite() function reads the time_map and takes the
   read lock, whereas the time_map_fread() function takes the write
   lock.
*/
void time_map_fwrite(time_map_type *map, const char *filename) {
    pthread_rwlock_rdlock(&map->rw_lock);
    {
        if (map->modified) {
            auto stream = mkdir_fopen(fs::path(filename), "w");
            time_t_vector_fwrite(map->map, stream);
            fclose(stream);
        }
        map->modified = false;
    }
    pthread_rwlock_unlock(&map->rw_lock);
}

void time_map_fread(time_map_type *map, const char *filename) {
    time_map_assert_writable(map);
    pthread_rwlock_wrlock(&map->rw_lock);
    {
        if (fs::exists(filename)) {
            FILE *stream = util_fopen(filename, "r");
            time_t_vector_type *file_map = time_t_vector_fread_alloc(stream);

            for (int step = 0; step < time_t_vector_size(file_map); step++)
                time_map_update__(map, step,
                                  time_t_vector_iget(file_map, step));

            time_t_vector_free(file_map);
            fclose(stream);
        }
    }
    pthread_rwlock_unlock(&map->rw_lock);
    time_map_get_last_step(map);
    map->modified = false;
}

/**
  Observe that the return value from this function is an inclusive
  value; i.e. it should be permissible to ask for results at this report
  step.
*/
int time_map_get_last_step(time_map_type *map) {
    int last_step;

    pthread_rwlock_rdlock(&map->rw_lock);
    last_step = time_t_vector_size(map->map) - 1;
    pthread_rwlock_unlock(&map->rw_lock);

    return last_step;
}

int time_map_get_size(time_map_type *map) {
    return time_map_get_last_step(map) + 1;
}

time_t time_map_get_start_time(time_map_type *map) {
    return time_map_iget(map, 0);
}

time_t time_map_get_end_time(time_map_type *map) {
    int last_step = time_map_get_last_step(map);
    return time_map_iget(map, last_step);
}

double time_map_get_end_days(time_map_type *map) {
    int last_step = time_map_get_last_step(map);
    return time_map_iget_sim_days(map, last_step);
}

bool time_map_update(time_map_type *map, int step, time_t time) {
    bool updateOK = time_map_try_update(map, step, time);
    if (!updateOK)
        time_map_update_abort(map, step, time);
    return updateOK;
}

bool time_map_try_update(time_map_type *map, int step, time_t time) {
    bool updateOK;
    time_map_assert_writable(map);
    pthread_rwlock_wrlock(&map->rw_lock);
    { updateOK = time_map_update__(map, step, time); }
    pthread_rwlock_unlock(&map->rw_lock);
    return updateOK;
}

bool time_map_summary_update(time_map_type *map, const ecl_sum_type *ecl_sum) {
    bool updateOK = time_map_try_summary_update(map, ecl_sum);

    if (!updateOK) {
        time_map_summary_log_mismatch(map, ecl_sum);
    }

    return updateOK;
}

bool time_map_try_summary_update(time_map_type *map,
                                 const ecl_sum_type *ecl_sum) {
    bool updateOK;

    time_map_assert_writable(map);
    pthread_rwlock_wrlock(&map->rw_lock);
    { updateOK = time_map_summary_update__(map, ecl_sum); }
    pthread_rwlock_unlock(&map->rw_lock);

    return updateOK;
}

int time_map_lookup_time(time_map_type *map, time_t time) {
    int index = -1;
    pthread_rwlock_rdlock(&map->rw_lock);
    {
        int current_index = 0;
        while (true) {
            if (current_index >= time_t_vector_size(map->map))
                break;

            if (time_map_iget__(map, current_index) == time) {
                index = current_index;
                break;
            }

            current_index++;
        }
    }
    pthread_rwlock_unlock(&map->rw_lock);
    return index;
}

static bool time_map_valid_time__(const time_map_type *map, time_t time) {
    if (time_t_vector_size(map->map) > 0) {
        if ((time >= time_map_iget__(map, 0)) &&
            (time <= time_map_iget__(map, time_t_vector_size(map->map) - 1)))
            return true;
        else
            return false;
    } else
        return false;
}

int time_map_lookup_time_with_tolerance(time_map_type *map, time_t time,
                                        int seconds_before_tolerance,
                                        int seconds_after_tolerance) {
    int nearest_index = -1;
    pthread_rwlock_rdlock(&map->rw_lock);
    {
        if (time_map_valid_time__(map, time)) {
            time_t nearest_diff = 999999999999;
            int current_index = 0;
            while (true) {
                time_t diff = time - time_map_iget__(map, current_index);
                if (diff == 0) {
                    nearest_index = current_index;
                    break;
                }

                if (std::fabs(diff) < nearest_diff) {
                    bool inside_tolerance = true;
                    if (seconds_after_tolerance >= 0) {
                        if (diff >= seconds_after_tolerance)
                            inside_tolerance = false;
                    }

                    if (seconds_before_tolerance >= 0) {
                        if (diff <= -seconds_before_tolerance)
                            inside_tolerance = false;
                    }

                    if (inside_tolerance) {
                        nearest_diff = diff;
                        nearest_index = current_index;
                    }
                }

                current_index++;

                if (current_index >= time_t_vector_size(map->map))
                    break;
            }
        }
    }
    pthread_rwlock_unlock(&map->rw_lock);
    return nearest_index;
}

int time_map_lookup_days(time_map_type *map, double sim_days) {
    int index = -1;
    pthread_rwlock_rdlock(&map->rw_lock);
    {
        if (time_t_vector_size(map->map) > 0) {
            time_t time = time_map_iget__(map, 0);
            util_inplace_forward_days_utc(&time, sim_days);
            index = time_map_lookup_time(map, time);
        }
    }
    pthread_rwlock_unlock(&map->rw_lock);
    return index;
}

void time_map_clear(time_map_type *map) {
    time_map_assert_writable(map);
    pthread_rwlock_wrlock(&map->rw_lock);
    {
        time_t_vector_reset(map->map);
        map->modified = true;
    }
    pthread_rwlock_unlock(&map->rw_lock);
}

static void time_map_update_abort(time_map_type *map, int step, time_t time) {
    time_t current_time = time_map_iget__(map, step);
    int current[3];
    int new_time[3];

    util_set_date_values_utc(current_time, &current[0], &current[1],
                             &current[2]);
    util_set_date_values_utc(time, &new_time[0], &new_time[1], &new_time[2]);

    util_abort("%s: time mismatch for step:%d   New_Time: %02d/%02d/%04d   "
               "existing: %02d/%02d/%04d \n",
               __func__, step, new_time[0], new_time[1], new_time[2],
               current[0], current[1], current[2]);
}

static void time_map_summary_log_mismatch(time_map_type *map,
                                          const ecl_sum_type *ecl_sum) {
    // If the normal summary update fails we just play through all time steps
    // to pinpoint exactly the step where the update fails.
    int first_step = ecl_sum_get_first_report_step(ecl_sum);
    int last_step = ecl_sum_get_last_report_step(ecl_sum);
    int step;
    std::string error_msg;
    for (step = first_step; step <= last_step; step++) {
        if (ecl_sum_has_report_step(ecl_sum, step)) {
            time_t time = ecl_sum_get_report_time(ecl_sum, step);
            auto fm_time = *std::localtime(&time);

            if (map->refcase) {
                if (ecl_sum_get_last_report_step(ecl_sum) >= step) {
                    if (ecl_sum_has_report_step(map->refcase, step)) {
                        time_t ref_time =
                            ecl_sum_get_report_time(map->refcase, step);
                        if (ref_time != time) {
                            auto ref_case_time = *std::localtime(&ref_time);
                            error_msg.append(fmt::format(
                                "Time mismatch for step: {}, new time: "
                                "{}, reference case: {}\n",
                                step, std::put_time(&fm_time, "%Y-%m-%d"),
                                std::put_time(&ref_case_time, "%Y-%m-%d")));
                        }
                    } else {
                        error_msg.append(fmt::format(
                            "Missing step: {} in refcase at time: {}", step,
                            std::put_time(&fm_time, "%Y-%m-%d")));
                    }
                }
            } else {
                time_t current_time = time_map_iget__(map, step);
                if (current_time != time) {
                    auto current_time_tm = *std::localtime(&current_time);
                    error_msg.append(fmt::format(
                        "Time mismatch for step: {}, new time: "
                        "{}, existing: {}\n",
                        step, std::put_time(&current_time_tm, "%Y-%m-%d"),
                        std::put_time(&fm_time, "%Y-%m-%d")));
                }
            }
        }
    }
    logger->error(
        "Inconsistency in time_map - loading SUMMARY from: {} failed:\n{}",
        ecl_sum_get_path(ecl_sum), error_msg);
}

/**
  This function creates an integer index mapping from the time map
  into the summary case. In general the time <-> report step mapping
  of the summary data should coincide exactly with the one maintained
  in the time_map, however we allow extra timesteps in the summary
  instance. The extra timesteps will be ignored, holes in the summary
  timestep is not allowed - that will lead to a hard crash.

     time map                      Summary
     -------------------------------------------------
     0: 01/01/2000   <-------      0: 01/01/2000

     1: 01/02/2000   <-------      1: 01/02/2000

     2: 01/03/2000   <-\           2: 02/02/2000 (Ignored)
                        \
                         \--       3: 01/03/2000

     3: 01/04/2000   <-------      4: 01/04/2000


     index_map = { 0 , 1 , 3 , 4 }

  Observe that the time_map_update_summary() must be called prior to
  calling this function, to ensure that the time_map is sufficiently
  long. If timesteps are missing from the summary case we crash hard:


     time map                      Summary
     -------------------------------------------------
     0: 01/01/2000   <-------      0: 01/01/2000

     1: 01/02/2000   <-------      1: 01/02/2000

     2: 01/03/2000                 ## ERROR -> util_abort()

     3: 01/04/2000   <-------      2: 01/04/2000

*/
int_vector_type *time_map_alloc_index_map(time_map_type *map,
                                          const ecl_sum_type *ecl_sum) {
    int_vector_type *index_map = int_vector_alloc(0, -1);
    pthread_rwlock_rdlock(&map->rw_lock);

    int sum_index = ecl_sum_get_first_report_step(ecl_sum);
    int time_map_index = ecl_sum_get_first_report_step(ecl_sum);
    for (; time_map_index < time_map_get_size(map); ++time_map_index) {
        time_t map_time = time_map_iget__(map, time_map_index);
        if (map_time == DEFAULT_TIME)
            continue;

        for (; sum_index <= ecl_sum_get_last_report_step(ecl_sum);
             ++sum_index) {
            time_t sum_time = ecl_sum_get_report_time(ecl_sum, sum_index);
            if (sum_time == map_time)
                break;

            if (sum_time > map_time) {
                int day, month, year;
                util_set_date_values_utc(map_time, &day, &month, &year);
                util_abort("%s: The eclipse summary cases is missing data for "
                           "date:%02d/%02d/%4d - aborting\n",
                           __func__, day, month, year);
            }
        }

        if (sum_index > ecl_sum_get_last_report_step(ecl_sum)) {
            logger->error("Inconsistency in time_map - data will be ignored");
            break;
        }

        int_vector_iset(index_map, time_map_index, sum_index);
    }

    pthread_rwlock_unlock(&map->rw_lock);
    return index_map;
}

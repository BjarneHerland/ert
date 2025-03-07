# -----------------------------------------------------------------
# Additional dependencies
# -----------------------------------------------------------------

find_program(SBATCH "sbatch")

file(COPY "${TEST_DATA_DIR}/job_queue/qsub_emulators/"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

# -----------------------------------------------------------------
# Public tests
# -----------------------------------------------------------------

foreach(
  name
  job_status
  ext_job
  job_node
  job_lsf_parse_bsub_stdout
  job_lsf
  job_queue_driver
  job_slurm_driver
  job_mock_slurm
  job_torque)

  add_executable(${name} test_${name}.cpp)
  target_link_libraries(${name} res)
  add_test(NAME ${name} COMMAND ${name})
endforeach()

foreach(name job_slurm_submit job_slurm_runtest)
  add_executable(${name} test_${name}.cpp)
  target_link_libraries(${name} res)
  if(SBATCH)
    add_test(NAME ${name} COMMAND ${name})
  endif()
endforeach()

foreach(
  name
  job_loadOK
  ext_joblist
  job_torque_submit
  job_queue_stress_task
  job_program_output
  job_lsf_exclude_hosts
  job_workflow
  create_file
  job_loadFail)

  add_executable(${name} test_${name}.cpp)
  target_link_libraries(${name} res)
endforeach()

if(NOT APPLE)
  add_executable(job_list test_job_list.cpp)
  target_link_libraries(job_list res)

  add_test(NAME job_list COMMAND $<TARGET_FILE:job_list>)
endif()

add_test(NAME job_loadOK1 COMMAND job_loadOK
                                  "${TEST_DATA_DIR}/job_queue/internalOK")
add_test(NAME job_loadOK2 COMMAND job_loadOK
                                  "${TEST_DATA_DIR}/job_queue/externalOK")
add_test(NAME job_loadOK3
         COMMAND job_loadOK "${TEST_DATA_DIR}/job_queue/internalOK"
                 "${TEST_DATA_DIR}/job_queue/externalOK")

add_test(NAME job_loadFail1 COMMAND job_loadFail
                                    "${TEST_DATA_DIR}/job_queue/internalFail")
add_test(NAME job_loadFail2 COMMAND job_loadFail
                                    "${TEST_DATA_DIR}/job_queue/externalFail")
add_test(NAME job_loadFail3
         COMMAND job_loadFail "${TEST_DATA_DIR}/job_queue/internalFail"
                 "${TEST_DATA_DIR}/job_queue/externalFail")

set_target_properties(job_workflow PROPERTIES ENABLE_EXPORTS ON)
add_test(NAME job_workflow COMMAND job_workflow "${CMAKE_CURRENT_BINARY_DIR}"
                                   "${TEST_DATA_DIR}/job_queue/internal_job")

add_test(NAME job_lsf_exclude_hosts COMMAND job_lsf_exclude_hosts job_program
                                            NULL LOCAL)

add_test(
  NAME job_torque_submit
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND job_torque_submit dummyparam)
set_property(TEST job_torque_submit PROPERTY ENVIRONMENT “setenv PATH
                                             ${CMAKE_CURRENT_BINARY_DIR}:$PATH”)

add_test(NAME ext_joblist
         COMMAND ext_joblist "${TEST_DATA_DIR}/job_queue/jobs/util"
                 "${CMAKE_CURRENT_SOURCE_DIR}")

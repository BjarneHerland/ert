add_executable(rms_file_test test_rms_file.cpp)
target_link_libraries(rms_file_test res)

if(NOT EXISTS "${EQUINOR_TEST_DATA_DIR}")
  return()
endif()

add_test(NAME rms_file_test
         COMMAND rms_file_test
                 "${EQUINOR_TEST_DATA_DIR}/config/rms_file_test/rms_file")

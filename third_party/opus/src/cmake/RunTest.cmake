if(NOT EXISTS ${TEST_EXECUTABLE})
    message(FATAL_ERROR "Error could not find ${TEST_EXECUTABLE}, ensure that you built the test binary")
endif()

# Run the test binary on the host.
execute_process(COMMAND ${TEST_EXECUTABLE} RESULT_VARIABLE CMD_RESULT)
if(CMD_RESULT)
  message(FATAL_ERROR "Error running ${TEST_EXECUTABLE} failed with result ${CMD_RESULT}")
endif()

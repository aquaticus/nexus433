get_filename_component(SRC_DIR ${SRC} DIRECTORY)
if(GIT_EXECUTABLE AND NOT DEFINED VERSION)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --match "v*"
    WORKING_DIRECTORY ${SRC_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    RESULT_VARIABLE ERROR
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(NOT ERROR)
    set(VERSION ${GIT_VERSION})
  endif()
endif()

if(NOT DEFINED VERSION)
  set(VERSION unknown)
endif()

configure_file(${SRC} ${DST} @ONLY)

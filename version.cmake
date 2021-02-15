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

message(STATUS "Software version: ${VERSION}")
configure_file(${SRC} ${DST} @ONLY)

# no easy way to pass version to CPack, separate script is used to build package
string(REGEX REPLACE "^v"  "" PACKAGE_VERSION ${GIT_VERSION} )
set(PACKAGE_VERSION_BOARD "${PACKAGE_VERSION}-${BOARD}")
configure_file(${PKG_SRC} ${PKG_DST} @ONLY)
execute_process(COMMAND "chmod" "+x" ${PKG_DST})

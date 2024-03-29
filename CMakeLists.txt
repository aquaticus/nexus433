cmake_minimum_required(VERSION 3.1)
project (nexus433)

set(INSTALL_INI_DIR "${CMAKE_INSTALL_PREFIX}/etc" CACHE STRING "Directory where to look for default INI file" )
set(INSTALL_INI_FILENAME "nexus433.ini" CACHE STRING "Default INI file name" )
set(INSTALL_INI_PATH "${INSTALL_INI_DIR}/${INSTALL_INI_FILENAME}" ) 

option(ENABLE_FAKE_TRANSMITTER "Enable fake transmitter for diagnostic purposes" OFF)
IF(ENABLE_FAKE_TRANSMITTER)
  add_compile_definitions(ENABLE_FAKE_TRANSMITTER)
ENDIF()

option(DISABLE_DEPRECATED "Disable deprecated warning during compilation" ON)
if(DISABLE_DEPRECATED)
  add_compile_options(-Wno-deprecated-declarations)
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "RELEASE")
  # warnings as errors for release
  add_compile_options(-Werror)
  # exception for unknown pragmas (for gcc backward compatibility)
  add_compile_options(-Wno-unknown-pragmas)
endif()

file(GLOB SOURCES "*.cpp")
add_executable(nexus433 ${SOURCES})

include(board.cmake)

find_library(MOSQUITTOCPP_LIBRARY NAMES libmosquittopp.so)
find_library(MOSQUITTO_LIBRARY NAMES libmosquitto.so)
if(NOT MOSQUITTO_LIBRARY OR NOT MOSQUITTOCPP_LIBRARY)
  message(FATAL_ERROR "Mosquitto library not found. Install: apt-get install libmosquittopp-dev")
endif()

find_library(GPIOD_LIBRARY NAMES libgpiod.so)
if(NOT GPIOD_LIBRARY)
  message(FATAL_ERROR "gpiod library not found. Install apt install libgpiod-dev")
endif()

find_package(Threads REQUIRED)

set(LIBS
	${MOSQUITTOCPP_LIBRARY}
	${MOSQUITTO_LIBRARY}
	${GPIOD_LIBRARY}
	${CMAKE_THREAD_LIBS_INIT}
	)

target_link_libraries (nexus433  ${LIBS})

find_package(Git)
add_custom_target(version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/version.h.in
                   -D DST=${CMAKE_BINARY_DIR}/version.h
                   -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                   -D PKG_SRC=${CMAKE_SOURCE_DIR}/build_debian_package.sh.in
                   -D PKG_DST=${CMAKE_BINARY_DIR}/build_debian_package.sh
                   -D BOARD=${BOARD}
                   -P ${CMAKE_SOURCE_DIR}/version.cmake
  )

# for nexus433.ini.example
set(NEXUS433_BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
get_filename_component(NEXUS433_BINARY_PATH "${NEXUS433_BIN_INSTALL_DIR}/nexus433" ABSOLUTE)

configure_file(nexus433.ini.in nexus433.ini.example)
configure_file(board.h.in board.h)
configure_file(nexus433.service.in nexus433.service)

add_dependencies( nexus433 version )

set (CMAKE_CXX_STANDARD 11)
target_include_directories(nexus433 PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries (nexus433  ${LIBS})
target_compile_options(nexus433 PRIVATE "-Wall;-Wno-psabi" "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb;-DDEBUG>")

install(TARGETS nexus433 DESTINATION ${NEXUS433_BIN_INSTALL_DIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nexus433.ini.example" DESTINATION "${INSTALL_INI_DIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nexus433.service" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/systemd/system/")

include(debian_package.cmake)
include(CPack)

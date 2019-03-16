cmake_minimum_required(VERSION 3.1)
project (nexus433)

set(INSTALL_INI_DIR "/etc" CACHE STRING "Directory where INI file will be installed" )
set(INSTALL_INI_FILENAME "nexus433.ini" CACHE STRING "Installed config INI file name" )
set(INSTALL_INI_PATH "${INSTALL_INI_DIR}/${INSTALL_INI_FILENAME}" ) 

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
  message(FATAL_ERROR "gpiod library not found. Install https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/")
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
                   -P ${CMAKE_SOURCE_DIR}/version.cmake
  )

get_filename_component(NEXUS433_BINARY_PATH "${CMAKE_INSTALL_PREFIX}/bin/nexus433" ABSOLUTE)
configure_file(nexus433.ini.in nexus433.ini)
configure_file(board.h.in board.h)
configure_file(nexus433.service.in nexus433.service)

add_dependencies( nexus433 version )

set (CMAKE_CXX_STANDARD 11)
target_include_directories(nexus433 PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries (nexus433  ${LIBS})
target_compile_options(nexus433 PRIVATE "-Wall;-Wno-psabi" "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb;-DDEBUG>")

install(TARGETS nexus433 DESTINATION ${NEXUS433_BINARY_PATH})
install(CODE "set(INSTALL_INI_DIR ${INSTALL_INI_DIR})")
install(CODE "set(INSTALL_INI_FILENAME ${INSTALL_INI_FILENAME})")
install(SCRIPT install_config.cmake)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nexus433.service" DESTINATION "/etc/systemd/system/")

# Installs INI file. It will not override existing configuration.

set(INSTALL_INI_PATH "${INSTALL_INI_DIR}/${INSTALL_INI_FILENAME}")

if( EXISTS "${INSTALL_INI_PATH}" )
	set(OUT_INI "${INSTALL_INI_PATH}.install")
	message(STATUS "Configuration file already exists. New config stored as ${OUT_INI}")
else()
	set(OUT_INI "${INSTALL_INI_PATH}")
	message(STATUS "Copy configuration file to ${INSTALL_INI_PATH}")
endif()

execute_process(COMMAND "cp" "nexus433.ini" "${INSTALL_INI_PATH}"
				OUTPUT_VARIABLE OUT 
				ERROR_VARIABLE OUT
				RESULT_VARIABLE RESULT)
if(NOT ${RESULT} EQUAL 0)
	message(FATAL_ERROR "${OUT}") 
endif()

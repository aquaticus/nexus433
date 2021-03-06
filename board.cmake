set( GPIOD_DEFAULT_PIN "1" CACHE STRING "Default pin number where 433MHz receiver is connected")
set( GPIOD_DEFAULT_DEVICE "/dev/gpiochip0" CACHE STRING "Default device name where 433MHz receiver is connected")

if( NOT BOARD )
	message(STATUS "Check for board type")
	if(EXISTS "/sys/firmware/devicetree/base/model")
		file(READ "/sys/firmware/devicetree/base/model" MODEL)
	else()
		set(MODEL "unknown")
	endif()
	
	string(FIND ${MODEL} "Raspberry" POS)
	if( ${POS} GREATER -1 )
		set(BOARD "raspberrypi")
	else()
		string(FIND ${MODEL} "Orange Pi" POS)
		if( ${POS} GREATER -1 )  	
		  	set(BOARD "orangepi")
		 endif()
	endif()
endif()

message(STATUS "Board: ${BOARD}")
if( "${BOARD}" STREQUAL raspberrypi )
	set(GPIOD_DEFAULT_PIN 17)
elseif( "${BOARD}" STREQUAL orangepi )
	set(GPIOD_DEFAULT_PIN 1)
else()
	message(WARNING "Unknown board ${BOARD}. Using default pin number for 433MHz receiver. You can ignore this warning.")
endif()

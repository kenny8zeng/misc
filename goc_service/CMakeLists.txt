cmake_minimum_required( VERSION 3.0.0 )
project( goc_service )

add_subdirectory( serialport )
set( INCLUDE_DIR_LIST ${INCLUDE_DIR_LIST}
	${CMAKE_CURRENT_LIST_DIR}/serialport
)

add_subdirectory( function )
set( INCLUDE_DIR_LIST ${INCLUDE_DIR_LIST}
	${CMAKE_CURRENT_LIST_DIR}/function
)


include_directories( ${INCLUDE_DIR_LIST} )

set( SCR_FILES ${SCR_FILES}
	main.cpp
)

add_executable( goc_service ${SCR_FILES} )

target_link_libraries( goc_service function serialport )


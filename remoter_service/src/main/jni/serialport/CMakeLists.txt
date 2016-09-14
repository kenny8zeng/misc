
set( SRC_FILE_LIST ${SRC_FILE_LIST}
	./serialport.cpp
	./serialport.hpp
)

add_library( serialport SHARED ${SRC_FILE_LIST} )


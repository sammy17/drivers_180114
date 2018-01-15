cmake_minimum_required(VERSION 3.5)
project(node)

set(CMAKE_CXX_STANDARD 11)
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS})


file(GLOB SOURCE_FILES
	"main.cpp"
	"drivers/*.c"
	"drivers/*.h"
	"include/xparameters.h"
     )

add_executable(node ${SOURCE_FILES})

target_link_libraries( node ${OpenCV_LIBS} -lpthread)

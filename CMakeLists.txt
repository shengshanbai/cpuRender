cmake_minimum_required(VERSION 3.10.0)

project(cpuRender)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(SOURCE main.cpp
tiny_obj_loader.h
tiny_obj_loader.cpp
ObjModel.h
ObjModel.cpp
utils.h
utils.cpp
log.h
log.cpp
render.h
render.cpp
CompilerSpecific.h
)

add_compile_options(-mavx2 -mfma -msse4.1)

ADD_EXECUTABLE(cpuRender ${SOURCE})
target_link_libraries(cpuRender ${OpenCV_LIBS})

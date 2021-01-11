cmake_minimum_required(VERSION 3.10.0)
project(cpuRender)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(SOURCE main.cpp
MaskedOcclutionCulling.cpp)

ADD_EXECUTABLE(cpuRender ${SOURCE})
target_link_libraries(cpuRender ${OpenCV_LIBS})
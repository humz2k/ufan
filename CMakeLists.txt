cmake_minimum_required(VERSION 3.23)
project(
  ufan
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(quill REQUIRED)

add_library(ufan INTERFACE)
target_include_directories(ufan INTERFACE ${PROJECT_SOURCE_DIR}/include)

file(GLOB DRIVERS_SOURCES ${PROJECT_SOURCE_DIR}/drivers/*.cpp)
foreach(sourcefile ${DRIVERS_SOURCES})
  get_filename_component(name ${sourcefile} NAME_WE)
  add_executable(ufan-${name} ${sourcefile})
  target_link_libraries(ufan-${name} ufan quill::quill)
endforeach(sourcefile ${DRIVERS_SOURCES})

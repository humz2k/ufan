cmake_minimum_required(VERSION 3.23)
project(
  ufan
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(quill REQUIRED)

add_library(ufan)
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")
target_sources(ufan PRIVATE ${SOURCE_FILES})
target_include_directories(ufan PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ufan PUBLIC quill::quill)

file(GLOB DRIVERS_SOURCES ${PROJECT_SOURCE_DIR}/drivers/*.cpp)
foreach(sourcefile ${DRIVERS_SOURCES})
  get_filename_component(name ${sourcefile} NAME_WE)
  add_executable(ufan-${name} ${sourcefile})
  target_link_libraries(ufan-${name} ufan)
endforeach(sourcefile ${DRIVERS_SOURCES})

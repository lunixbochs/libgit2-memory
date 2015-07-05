cmake_minimum_required(VERSION 2.6)

project(git2-memory)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
include_directories(src/memory)

add_definitions(-ggdb -O2)

file(GLOB_RECURSE MEMORY_SOURCE src/memory/*.c)
add_library(git2-memory STATIC ${MEMORY_SOURCE})

file(GLOB_RECURSE EXAMPLE_SOURCE src/example/*.c)
add_executable(example ${EXAMPLE_SOURCE})
target_link_libraries(example git2-memory git2)

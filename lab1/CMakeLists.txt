cmake_minimum_required(VERSION 3.0.0)
project(os_lab1 VERSION 0.1.0)

# trial functions library
add_subdirectory("../trialfuncs" "trialfuncs")

# Support library
add_library(eraha shared_data.c)
target_include_directories(eraha PRIVATE "../trialfuncs/include")

# Manager
add_executable(manager main.c manager.c)
target_link_libraries(manager PRIVATE eraha lab1)

# Task
add_executable(calculon calculon.c)
target_link_libraries(calculon PRIVATE eraha lab1)


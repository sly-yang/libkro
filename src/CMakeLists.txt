cmake_minimum_required(VERSION 3.10)
project(libkro C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE Release)

option(BUILD_EXAMPLES "Build example programs" OFF)

# Static library
add_library(kro STATIC kro.c)
target_include_directories(kro PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
    target_link_libraries(kro ws2_32)
endif()

# Install rules
install(TARGETS kro
        ARCHIVE DESTINATION lib)
install(FILES include/kro.h
        DESTINATION include)

# Build examples if enabled
if(BUILD_EXAMPLES)
    message(STATUS "Building example programs")

    add_executable(kro_write_example example/write_example.c)
    target_link_libraries(kro_write_example kro)
    target_include_directories(kro_write_example PRIVATE include)

    add_executable(kro_read_example example/read_example.c)
    target_link_libraries(kro_read_example kro)
    target_include_directories(kro_read_example PRIVATE include)
endif()

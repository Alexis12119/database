cmake_minimum_required(VERSION 3.12)
project(database CXX)

# Retrieve the value of the CPLUS_INCLUDE_PATH environment variable
if(DEFINED ENV{CPLUS_INCLUDE_PATH})
    set(CPLUS_INCLUDE_PATH ENV{CPLUS_INCLUDE_PATH})
endif()

# Add the paths from CPLUS_INCLUDE_PATH to include directories
include_directories(
    include
    CPLUS_INCLUDE_PATH
    # Add any other include paths specific to your project here
)

# Add your source files
add_executable(main main.cpp)

# Specify the library directories
link_directories(lib)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Generate compile_commands.json in the build directory

cmake_minimum_required(VERSION 3.30)
project(LLRI2_Experiments_Vulkan_Helpers)

# Set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Creates the TARGET for this project, which is the executable.
# The commands after that require the TARGET to be created first.

# The WIN32 option is used to create a window without a console on Windows.
# It is safe to use on other platforms as well, where it won't do anything
add_executable("${PROJECT_NAME}" WIN32 src/main.cpp)

# Set C++ version to 20
target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_20)

# Link to the actual SDL3 library.
target_link_libraries("${PROJECT_NAME}" PRIVATE SDL3::SDL3)

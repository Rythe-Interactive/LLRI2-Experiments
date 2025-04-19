cmake_minimum_required(VERSION 3.30)
project(LLRI2_Experiments_Vulkan_Helpers)

# Creates the TARGET for this project, which is the executable.
# The commands after that require the TARGET to be created first.

## The WIN32 option is used to create a window without a console on Windows.
## It is safe to use on other platforms as well, where it won't do anything
add_executable(${PROJECT_NAME} WIN32 src/main.cpp)

# Set C++ version
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

# Link to the SDL3 library.
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

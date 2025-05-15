cmake_minimum_required(VERSION 3.30)
project(LLRI2_Experiments_SDL3_GPU)
add_compile_definitions(MYPROJECT_NAME=${PROJECT_NAME})

# Creates the TARGET for this project, which is the executable.
# The commands after that require the TARGET to be created first.

## The WIN32 option is used to create a window without a console on Windows.
## It is safe to use on other platforms as well, where it won't do anything
add_executable(${PROJECT_NAME} WIN32 src/main.cpp)

# Set C++ version
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

# Link to the SDL3 library.
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

# Link to the RSL library.
target_link_libraries(${PROJECT_NAME} PRIVATE rsl)

# Link to the Assimp library.
target_link_libraries(${PROJECT_NAME} PRIVATE assimp)

# Copy assets to the build output directory after building.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}-assets"
)

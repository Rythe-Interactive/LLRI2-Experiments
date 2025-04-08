cmake_minimum_required(VERSION 3.30)
project(LLRI2_Experiments)

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

# RSL Math
set(RSL_DIR vendored/Rythe-Standard-Library)
include_directories(${RSL_DIR}/src/rsl/impl/math)

add_subdirectory(SDL3_GPU)
add_subdirectory(Vulkan_Raw)
add_subdirectory(Vulkan_Helpers)

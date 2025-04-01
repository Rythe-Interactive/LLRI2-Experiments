cmake_minimum_required(VERSION 3.30)
project(LLRI2_Experiments)

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

add_subdirectory(SDL3_GPU)
add_subdirectory(Vulkan_Raw)
add_subdirectory(Vulkan_Helpers)

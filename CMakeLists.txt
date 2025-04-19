cmake_minimum_required(VERSION 3.30)
project(LLRI2_Experiments)

# Libraries

# Set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

## SDL3
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

## RSL
set(RSL_DIR ${CMAKE_SOURCE_DIR}/vendored/Rythe-Standard-Library)
file(GLOB_RECURSE RSL_SOURCES "${RSL_DIR}/src/*")
add_library(rsl STATIC ${RSL_SOURCES})

target_sources(rsl PUBLIC ${RSL_SOURCES})

### Due to the lack of file extensions, we need to tell CMake manually what to compile it as.
set_target_properties(rsl PROPERTIES LINKER_LANGUAGE CXX)

### Set C++ version
target_compile_features(rsl PUBLIC cxx_std_20)
set_target_properties(rsl PROPERTIES CXX_STANDARD_REQUIRED ON)

### Include directories
target_include_directories(rsl SYSTEM PUBLIC ${RSL_DIR}/src)
target_include_directories(rsl SYSTEM PUBLIC ${RSL_DIR}/third_party/spdlog/include)

### Enable CPU features
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(rsl PUBLIC -msse -mfma)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(rsl PUBLIC -msse -mfma)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(rsl PUBLIC /Oi)
endif()

# Precompiled headers are used to speed up compilation.
# target_precompile_headers(rsl INTERFACE ${RSL_DIR}/src/rsl/math) #TODO: Does not work...

## Assimp
add_subdirectory(vendored/assimp EXCLUDE_FROM_ALL)

# Projects
add_subdirectory(SDL3_GPU)
add_subdirectory(Vulkan_Raw)
add_subdirectory(Vulkan_Helpers)

cmake_minimum_required(VERSION 3.30)
project(LLRI2_Experiments)

# Libraries

## SDL3
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

## RSL Math
set(RSL_DIR ${CMAKE_SOURCE_DIR}/vendored/Rythe-Standard-Library/)

#file(GLOB RSL_HEADERS LIST_DIRECTORIES false ${RSL_DIR}/src/rsl/*)
#add_library(RSL ${RSL_HEADERS})

add_library(RSL INTERFACE)

### Due to the lack of file extensions, we need to tell CMake manually what to compile it as.
set_target_properties(RSL PROPERTIES LINKER_LANGUAGE CXX)

### Set C++ version
target_compile_features(RSL INTERFACE cxx_std_23)

#target_include_directories(RSL
#	INTERFACE
#	${RSL_DIR}/src/rsl/impl/math
#)

target_sources(RSL PUBLIC
	FILE_SET HEADERS
		BASE_DIRS ${RSL_DIR}/src/rsl/impl/math/
		FILES
			${RSL_DIR}/src/rsl/impl/math/color.hpp
			${RSL_DIR}/src/rsl/impl/math/constants.hpp
			${RSL_DIR}/src/rsl/impl/math/geometry.hpp
)

# Precompiled headers are used to speed up compilation.
#target_precompile_headers(RSL INTERFACE ${RSL_DIR}/src/rsl/math) #TODO: Does not work...

# Projects
add_subdirectory(SDL3_GPU)
add_subdirectory(Vulkan_Raw)
add_subdirectory(Vulkan_Helpers)

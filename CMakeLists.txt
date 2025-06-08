cmake_minimum_required(VERSION 3.30)
project(LLRI2_Experiments)

# Libraries

# Set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

## SDL3
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

## Assimp
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(vendored/assimp EXCLUDE_FROM_ALL)

## Volk
add_subdirectory(vendored/volk EXCLUDE_FROM_ALL)

## VkBootstrap
add_subdirectory(vendored/vk-bootstrap EXCLUDE_FROM_ALL)

## Vulkan Memory Allocator (VMA)
add_subdirectory(vendored/VulkanMemoryAllocator EXCLUDE_FROM_ALL)

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
if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
	target_compile_options(rsl PUBLIC -msse -mfma)
elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
	target_compile_options(rsl PUBLIC /Oi /Ob1)
endif ()

# Precompiled headers are used to speed up compilation.
# target_precompile_headers(rsl INTERFACE ${RSL_DIR}/src/rsl/math) #TODO: Does not work...

## Dear ImGui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendored/imgui)
set(IMGUI_SOURCES
		${IMGUI_DIR}/imgui.cpp
		${IMGUI_DIR}/imgui_draw.cpp
		${IMGUI_DIR}/imgui_demo.cpp
		${IMGUI_DIR}/imgui_tables.cpp
		${IMGUI_DIR}/imgui_widgets.cpp
		# Specific implementations:
		${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
		${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_sources(imgui PUBLIC ${IMGUI_SOURCES})

### Set C++ version
target_compile_features(imgui PUBLIC cxx_std_20)
set_target_properties(imgui PROPERTIES CXX_STANDARD_REQUIRED ON)

### Include directories
target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_DIR})
target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_DIR}/backends)

# Projects
add_subdirectory(SDL3_GPU)
add_subdirectory(Vulkan_Raw)
add_subdirectory(Vulkan_Helpers)

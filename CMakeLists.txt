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

## doomgeneric
set(DOOMGENERIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendored/doomgeneric/doomgeneric)
set(DOOMGENERIC_SOURCES
		${DOOMGENERIC_DIR}/dummy.c
		${DOOMGENERIC_DIR}/am_map.c
		${DOOMGENERIC_DIR}/doomdef.c
		${DOOMGENERIC_DIR}/doomstat.c
		${DOOMGENERIC_DIR}/dstrings.c
		${DOOMGENERIC_DIR}/d_event.c
		${DOOMGENERIC_DIR}/d_items.c
		${DOOMGENERIC_DIR}/d_iwad.c
		${DOOMGENERIC_DIR}/d_loop.c
		${DOOMGENERIC_DIR}/d_main.c
		${DOOMGENERIC_DIR}/d_mode.c
		${DOOMGENERIC_DIR}/d_net.c
		${DOOMGENERIC_DIR}/f_finale.c
		${DOOMGENERIC_DIR}/f_wipe.c
		${DOOMGENERIC_DIR}/g_game.c
		${DOOMGENERIC_DIR}/hu_lib.c
		${DOOMGENERIC_DIR}/hu_stuff.c
		${DOOMGENERIC_DIR}/info.c
		${DOOMGENERIC_DIR}/i_cdmus.c
		${DOOMGENERIC_DIR}/i_endoom.c
		${DOOMGENERIC_DIR}/i_joystick.c
		${DOOMGENERIC_DIR}/i_scale.c
		${DOOMGENERIC_DIR}/i_sound.c
		${DOOMGENERIC_DIR}/i_system.c
		${DOOMGENERIC_DIR}/i_timer.c
		${DOOMGENERIC_DIR}/memio.c
		${DOOMGENERIC_DIR}/m_argv.c
		${DOOMGENERIC_DIR}/m_bbox.c
		${DOOMGENERIC_DIR}/m_cheat.c
		${DOOMGENERIC_DIR}/m_config.c
		${DOOMGENERIC_DIR}/m_controls.c
		${DOOMGENERIC_DIR}/m_fixed.c
		${DOOMGENERIC_DIR}/m_menu.c
		${DOOMGENERIC_DIR}/m_misc.c
		${DOOMGENERIC_DIR}/m_random.c
		${DOOMGENERIC_DIR}/p_ceilng.c
		${DOOMGENERIC_DIR}/p_doors.c
		${DOOMGENERIC_DIR}/p_enemy.c
		${DOOMGENERIC_DIR}/p_floor.c
		${DOOMGENERIC_DIR}/p_inter.c
		${DOOMGENERIC_DIR}/p_lights.c
		${DOOMGENERIC_DIR}/p_map.c
		${DOOMGENERIC_DIR}/p_maputl.c
		${DOOMGENERIC_DIR}/p_mobj.c
		${DOOMGENERIC_DIR}/p_plats.c
		${DOOMGENERIC_DIR}/p_pspr.c
		${DOOMGENERIC_DIR}/p_saveg.c
		${DOOMGENERIC_DIR}/p_setup.c
		${DOOMGENERIC_DIR}/p_sight.c
		${DOOMGENERIC_DIR}/p_spec.c
		${DOOMGENERIC_DIR}/p_switch.c
		${DOOMGENERIC_DIR}/p_telept.c
		${DOOMGENERIC_DIR}/p_tick.c
		${DOOMGENERIC_DIR}/p_user.c
		${DOOMGENERIC_DIR}/r_bsp.c
		${DOOMGENERIC_DIR}/r_data.c
		${DOOMGENERIC_DIR}/r_draw.c
		${DOOMGENERIC_DIR}/r_main.c
		${DOOMGENERIC_DIR}/r_plane.c
		${DOOMGENERIC_DIR}/r_segs.c
		${DOOMGENERIC_DIR}/r_sky.c
		${DOOMGENERIC_DIR}/r_things.c
		${DOOMGENERIC_DIR}/sha1.c
		${DOOMGENERIC_DIR}/sounds.c
		${DOOMGENERIC_DIR}/statdump.c
		${DOOMGENERIC_DIR}/st_lib.c
		${DOOMGENERIC_DIR}/st_stuff.c
		${DOOMGENERIC_DIR}/s_sound.c
		${DOOMGENERIC_DIR}/tables.c
		${DOOMGENERIC_DIR}/v_video.c
		${DOOMGENERIC_DIR}/wi_stuff.c
		${DOOMGENERIC_DIR}/w_checksum.c
		${DOOMGENERIC_DIR}/w_file.c
		${DOOMGENERIC_DIR}/w_main.c
		${DOOMGENERIC_DIR}/w_wad.c
		${DOOMGENERIC_DIR}/z_zone.c
		${DOOMGENERIC_DIR}/w_file_stdc.c
		${DOOMGENERIC_DIR}/i_input.c
		${DOOMGENERIC_DIR}/i_video.c
		${DOOMGENERIC_DIR}/doomgeneric.c
)
add_library(doomgeneric STATIC ${DOOMGENERIC_SOURCES})
target_sources(doomgeneric PUBLIC ${DOOMGENERIC_SOURCES})
target_include_directories(doomgeneric SYSTEM PUBLIC ${DOOMGENERIC_DIR})

# Projects
add_subdirectory(SDL3_GPU)
add_subdirectory(Vulkan_Raw)
add_subdirectory(Vulkan_Helpers)

# LLRI2 Experiments

This is a repo with multiple C++ projects, all written during
the exploration phase of the rewrite process of the LLRI.

These programs should all do the same thing, but using different APIs.
The goal is to compare the API paradigms and see which one is the best for the new LLRI.

This repo has two branches:
- [`comparison`](https://github.com/Rythe-Interactive/LLRI2-Experiments/tree/comparison): This branch has all experiments doing the exact same thing, so they can be compared better.
- [`doom`](https://github.com/Rythe-Interactive/LLRI2-Experiments/tree/doom): This branch has Doom (1993) ported to the Vulkan_Helpers experiment! It is fully playable, if you provide a WAD file.

## Cloning & Building

Make sure to clone this repository recursively,
to download all the used libraries from their submodules.

Then open the folder in an IDE, or use the normal CMake commands to generate and build the projects.

```bash
mkdir build
cmake -S . -B build  # Generate build files
cmake --build build --parallel  # Run build files
cd build

# Execute built executables
./LLRI2_Experiments_SDL3_GPU
./LLRI2_Experiments_Vulkan_Raw
./LLRI2_Experiments_Vulkan_Helpers
```

## SDL3's GPU API

Uses SDL3's GPU API (https://wiki.libsdl.org/SDL3/CategoryGPU)

The SDL3 GPU API is an opaque wrapper around the Vulkan API, and some other platforms' GPU APIs.

## Vulkan (Raw)

Uses only the Vulkan API, without any wrappers of helper libraries.

## Vulkan (Helpers)

Uses the Vulkan API, but with the help of some libraries and non-opaque wrappers.

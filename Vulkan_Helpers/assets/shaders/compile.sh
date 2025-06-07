#!/usr/bin/env bash

# Requires glslangValidator to be installed from glslang

for file in *.glsl; do
	glslangValidator --target-env vulkan1.3 -e main -o "compiled/${file/.glsl/.spv}" "$file"
done

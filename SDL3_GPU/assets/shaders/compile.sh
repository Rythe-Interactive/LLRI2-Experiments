#!/usr/bin/env bash

# Requires the shadercross CLI to be installed from SDL_shadercross

for file in *.hlsl; do
	shadercross "$file" --debug -o "compiled/${file/.hlsl/.spv}"
done

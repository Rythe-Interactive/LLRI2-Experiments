#version 460

//shader input
layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;

//output write
layout (location = 0) out vec4 outFragColor;


void main()
{
	outFragColor = inColor;
}

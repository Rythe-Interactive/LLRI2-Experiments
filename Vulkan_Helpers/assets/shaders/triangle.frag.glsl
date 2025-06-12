#version 450

//shader input
layout (location = 0) in vec2 inUV;

//output write
layout (location = 0) out vec4 outFragColor;

//texture to access
layout(set = 0, binding = 0) uniform sampler2D displayTexture;


void main()
{
	outFragColor = texture(displayTexture, inUV);
}

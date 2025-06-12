#version 460

#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec2 outUV;

struct Vertex {
	vec4 position;
	vec2 uv;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

//push constants block
layout(push_constant) uniform constants
{
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
	VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
	//load vertex data from device adress
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	//output data
	gl_Position = PushConstants.projectionMatrix * PushConstants.viewMatrix * PushConstants.modelMatrix * v.position;
	outUV = v.uv;
}

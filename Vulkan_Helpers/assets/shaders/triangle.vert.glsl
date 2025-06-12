#version 460

#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
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
	gl_Position = PushConstants.projectionMatrix * PushConstants.viewMatrix * PushConstants.modelMatrix * vec4(v.position, 1.0);
	outColor = v.color;
	outUV = vec2(v.uv_x, v.uv_y);
}

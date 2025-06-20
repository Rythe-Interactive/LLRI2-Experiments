#version 460

//size of a workgroup for compute
layout (local_size_x = 16, local_size_y = 16) in;

//descriptor bindings for the pipeline
layout(rgba16f, set = 0, binding = 0) uniform image2D image;
layout(rgba8, set = 0, binding = 1) uniform image2D screen;

void main()
{
	ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(image);
	if (texelCoord.x < size.x && texelCoord.y < size.y)
	{
		vec4 screenColor = imageLoad(screen, texelCoord);
		imageStore(image, texelCoord, screenColor);
	}
}

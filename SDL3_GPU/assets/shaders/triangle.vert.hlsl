cbuffer UniformBlock : register(b0, space1)
{
	column_major float4x4 model : packoffset(c0);
}
cbuffer UniformBlock : register(b1, space1)
{
	column_major float4x4 view : packoffset(c0);
}
cbuffer UniformBlock : register(b2, space1)
{
	column_major float4x4 projection : packoffset(c0);
};

struct Input
{
	float4 Position : SV_Position; //apparently you can just say float4, even though we send a float3..? oh well. saves a conversion step later on in the actual shader code TODO: Ask about this
	float2 TexCoord : TEXCOORD0;
};

struct Output
{
	float2 TexCoord : TEXCOORD0;
	float4 Position : SV_Position;
};

Output main(Input input)
{
	Output output;
	output.TexCoord = input.TexCoord;
	output.Position = mul(projection, mul(view, mul(model, input.Position)));
	return output;
}

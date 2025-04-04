struct Input
{
	float3 Position : TEXCOORD0;
	float2 TexCoord : TEXCOORD1;
	float4 Color : TEXCOORD2;
};

struct Output
{
	float4 Color : TEXCOORD0;
	float2 TexCoord : TEXCOORD1;
	float4 Position : SV_Position;
};

Output main(Input input)
{
	Output output;
	output.Color = input.Color;
	output.TexCoord = input.TexCoord;
	output.Position = float4(input.Position, 1.0f);
	return output;
}

struct Input
{
	float2 TexCoord : TEXCOORD1;
};

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

float4 main(Input input) : SV_Target0
{
	return Texture.Sample(Sampler, input.TexCoord);
}

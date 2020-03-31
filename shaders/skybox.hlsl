// Skybox shader

//------------------------------------------------------------------------------
// VERTEX SHADER CONSTANTS
cbuffer vs_globals : register(b0)
{
	matrix mat_combined;
};

//------------------------------------------------------------------------------
// TEXTURES
Texture2D tex_diffuse : register(t0);
SamplerState sampler_diffuse;

//------------------------------------------------------------------------------
// VERTEX SHADER INPUT
struct VS_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
};

//------------------------------------------------------------------------------
// VERTEX SHADER OUTPUT
struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

//------------------------------------------------------------------------------
// VERTEX SHADER
void vs_main(VS_INPUT In, out VS_OUTPUT Out)
{
	Out.pos = mul(float4(In.pos, 1), mat_combined);
	Out.tex = In.tex;
}

//------------------------------------------------------------------------------
// PIXEL SHADER
float4 ps_main(VS_OUTPUT In) : SV_TARGET
{
	return tex_diffuse.Sample(sampler_diffuse, In.tex);
}

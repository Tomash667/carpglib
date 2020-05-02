// Skybox shader

//------------------------------------------------------------------------------
// VERTEX SHADER CONSTANTS
cbuffer VsGlobals : register(b0)
{
	matrix matCombined;
};

//------------------------------------------------------------------------------
// TEXTURES
Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse;

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
void VsMain(VS_INPUT In, out VS_OUTPUT Out)
{
	Out.pos = mul(float4(In.pos, 1), matCombined);
	Out.tex = In.tex;
}

//------------------------------------------------------------------------------
// PIXEL SHADER
float4 PsMain(VS_OUTPUT In) : SV_TARGET
{
	return texDiffuse.Sample(samplerDiffuse, In.tex);
}

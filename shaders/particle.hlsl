cbuffer VsGlobals : register(b0)
{
	matrix matCombined;
};

Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse;

struct VS_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD;
	float4 color : COLOR;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
	float4 color : COLOR;
};

void VsMain(in VS_INPUT In, out VS_OUTPUT Out)
{
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.tex = In.tex;
	Out.color = In.color;
}

float4 PsMain(in VS_OUTPUT In) : SV_TARGET
{
	return texDiffuse.Sample(samplerDiffuse, In.tex) * In.color;
}

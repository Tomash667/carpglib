cbuffer vs_globals : register(b0)
{
	float4x4 matCombined;
	float4x4 matBones[32];
};

cbuffer ps_globals : register(b0)
{
	float4 color;
};

Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse;

//******************************************************************************
struct MESH_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

//******************************************************************************
struct ANI_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	half weight : BLENDWEIGHT0;
	int4 indices : BLENDINDICES0;
};

//******************************************************************************
struct MESH_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

//******************************************************************************
void vs_mesh(in MESH_INPUT In, out MESH_OUTPUT Out)
{
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.tex = In.tex;
}

//******************************************************************************
void vs_ani(in ANI_INPUT In, out MESH_OUTPUT Out)
{
	float3 pos = mul(float4(In.pos,1), matBones[In.indices[0]]) * In.weight;
	pos += mul(float4(In.pos,1), matBones[In.indices[1]]) * (1-In.weight);
	Out.pos = mul(float4(pos,1), matCombined);
	Out.tex = In.tex;
}

//******************************************************************************
float4 ps(in MESH_OUTPUT In) : SV_TARGET
{
	float4 tex = texDiffuse.Sample(samplerDiffuse, In.tex);
	return float4(color.rgb, tex.w * color.w);
}

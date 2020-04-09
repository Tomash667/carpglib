cbuffer VsGlobals : register(b0)
{
	float4x4 matCombined;
	float4x4 matBones[32];
};

cbuffer PsGlobals : register(b0)
{
	float4 color;
};

Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse : register(s0);

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
	float weight : BLENDWEIGHT0;
	uint4 indices : BLENDINDICES0;
};

//******************************************************************************
struct MESH_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

//******************************************************************************
void VsMesh(in MESH_INPUT In, out MESH_OUTPUT Out)
{
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.tex = In.tex;
}

//******************************************************************************
void VsAni(in ANI_INPUT In, out MESH_OUTPUT Out)
{
	float3 pos = mul(float4(In.pos,1), matBones[In.indices[0]]).xyz * In.weight;
	pos += mul(float4(In.pos,1), matBones[In.indices[1]]).xyz * (1-In.weight);
	Out.pos = mul(float4(pos,1), matCombined);
	Out.tex = In.tex;
}

//******************************************************************************
float4 PsMain(in MESH_OUTPUT In) : SV_TARGET
{
	float4 tex = texDiffuse.Sample(samplerDiffuse, In.tex);
	return float4(color.rgb, tex.w * color.w);
}

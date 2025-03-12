cbuffer VsLocals : register(b0)
{
	matrix matCombined;
	matrix matBones[64];
};

struct VsInput
{
    float3 pos : POSITION;
};

struct VsAniInput
{
    float3 pos : POSITION;
	float weight : BLENDWEIGHT0;
	uint4 indices : BLENDINDICES0;
};

struct VsOutput
{
    float4 pos : SV_POSITION;
	float4 depthPos : TEXCOORD0;
};

void VsMesh(VsInput In, out VsOutput Out)
{
	float3 pos = In.pos;
	Out.pos = mul(float4(pos,1), matCombined);
	Out.depthPos = Out.pos;
}

void VsAni(VsAniInput In, out VsOutput Out)
{
	float3 pos = mul(float4(In.pos,1), matBones[In.indices[0]]).xyz * In.weight;
	pos += mul(float4(In.pos,1), matBones[In.indices[1]]).xyz * (1-In.weight);
	Out.pos = mul(float4(pos,1), matCombined);
    Out.depthPos = Out.pos;
}

float4 PsMain(VsOutput In) : SV_TARGET
{
    float depth = In.depthPos.z / In.depthPos.w;
	return float4(depth, depth, depth, 1);
}

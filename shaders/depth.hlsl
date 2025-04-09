cbuffer VsLocals : register(b0)
{
    matrix matCombined;
    matrix matBones[64];
};

struct VsInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};

struct VsAniInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float weight : BLENDWEIGHT0;
    uint4 indices : BLENDINDICES0;
};

struct VsOutput
{
    float4 pos : SV_POSITION;
};

static const float bias = 0.005f;

void VsMesh(VsInput In, out VsOutput Out)
{
    float3 pos = In.pos - In.normal * bias;
    Out.pos = mul(float4(pos, 1), matCombined);
}

void VsAni(VsAniInput In, out VsOutput Out)
{
    float4 adjPos = float4(In.pos - In.normal * bias, 1);
    float3 pos = mul(adjPos, matBones[In.indices[0]]).xyz * In.weight;
    pos += mul(adjPos, matBones[In.indices[1]]).xyz * (1 - In.weight);
    Out.pos = mul(float4(pos, 1), matCombined);
}

float4 PsMain(VsOutput In) : SV_TARGET
{
    float depth = In.pos.z / In.pos.w;
    return float4(depth, depth, depth, 1);
}

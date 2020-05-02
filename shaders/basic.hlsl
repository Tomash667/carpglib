cbuffer VsGlobals : register(b0)
{
	matrix matCombined;
};

cbuffer PsGlobalsMesh : register(b0)
{
	float4 color;
};

cbuffer PsGlobalsColor : register(b0)
{
	float3 playerPos;
	float range;
	float falloff;
};

//-----------------------------------------------
// Mesh
float4 VsMesh(in float3 pos : POSITION) : SV_POSITION
{
	return mul(float4(pos,1), matCombined);
}

float4 PsMesh() : SV_TARGET
{
	return color;
}

//-----------------------------------------------
// Color
struct ColorVertex
{
	float3 pos : POSITION;
	float4 color : COLOR;
};

struct ColorVertexOutput
{
	float4 pos : SV_POSITION;
	float3 realPos : TEXCOORD0;
	float4 color : COLOR;
};

void VsColor(in ColorVertex In, out ColorVertexOutput Out)
{
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.realPos = In.pos;
	Out.color = In.color;
}

float4 PsColor(in ColorVertexOutput In) : SV_TARGET
{
	float dist = distance(In.realPos, playerPos);
	clip(range - falloff * dist);
	return float4(In.color.rgb, lerp(In.color.a, 0.f, falloff * dist/range));
}

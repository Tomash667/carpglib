cbuffer VsGlobals : register(b0)
{
	matrix matCombined;
};

cbuffer PsGlobals : register(b0)
{
	float4 color;
	float3 playerPos;
	float range;
};

//-----------------------------------------------
// Simple
float4 VsSimple(in float3 pos : POSITION) : SV_POSITION
{
	return mul(float4(pos,1), matCombined);
}

float4 PsSimple() : SV_TARGET
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
	float4 color : COLOR;
};

void VsColor(in ColorVertex In, out ColorVertexOutput Out)
{
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.color = In.color;
}

float4 PsColor(in ColorVertexOutput In) : SV_TARGET
{
	return In.color;
}

//-----------------------------------------------
// Area
struct AreaVertexOutput
{
	float4 pos : SV_POSITION;
	float3 realPos : TEXCOORD0;
};

void VsArea(in float3 pos : POSITION, out AreaVertexOutput Out)
{
	Out.pos = mul(float4(pos,1), matCombined);
	Out.realPos = pos;
}

float4 PsArea(in AreaVertexOutput In) : SV_TARGET
{
	float dist = distance(In.realPos, playerPos);
	clip(range - dist);
	return float4(color.rgb, lerp(color.a, 0.f, dist/range));
}

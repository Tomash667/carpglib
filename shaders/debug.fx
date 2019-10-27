float4x4 matCombined;
float4 color;
float4 playerPos;
float range;

//-----------------------------------------------
// Simple
float4 vs_simple(in float3 pos : POSITION) : POSITION
{
	return mul(float4(pos,1), matCombined);
}

float4 ps_simple() : COLOR0
{
	return color;
}

technique techSimple
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_simple();
		PixelShader = compile PS_VERSION ps_simple();
	}
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
	float4 pos : POSITION;
	float4 color : COLOR;
};

void vs_color(in ColorVertex In, out ColorVertexOutput Out)
{
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.color = In.color;
}

float4 ps_color(in ColorVertexOutput In) : COLOR0
{
	return In.color;
}

technique techColor
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_color();
		PixelShader = compile PS_VERSION ps_color();
	}
}

//-----------------------------------------------
// Area
struct AreaVertexOutput
{
	float4 pos : POSITION;
	float3 realPos : TEXCOORD0;
};

void vs_area(in float3 pos : POSITION, out AreaVertexOutput Out)
{
	Out.pos = mul(float4(pos,1), matCombined);
	Out.realPos = pos;
}

float4 ps_area(in AreaVertexOutput In) : COLOR0
{
	float dist = distance(In.realPos, playerPos);
	clip(range - dist);
	return float4(color.rgb, lerp(color.a, 0.f, dist/range));
}

technique techArea
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_area();
		PixelShader = compile PS_VERSION ps_area();
	}
}

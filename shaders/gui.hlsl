cbuffer VsGlobals : register(b0)
{
	float2 size;
};

cbuffer PsGlobals : register(b0)
{
	float grayscale;
};

Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse : register(s0);

struct VERTEX_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR;
};

struct VERTEX_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR;
};

void VsMain(in VERTEX_INPUT In, out VERTEX_OUTPUT Out)
{
	Out.pos.x = (In.pos.x / (size.x * 0.5f)) - 1.0f;
	Out.pos.y = -((In.pos.y / (size.y * 0.5f)) - 1.0f);
	Out.pos.z = 0.f;
	Out.pos.w = 1.f;
	Out.tex = In.tex;
	Out.color = In.color;
}

float4 PsMain(in VERTEX_OUTPUT In) : SV_TARGET
{
	float4 c = texDiffuse.Sample(samplerDiffuse, In.tex) * In.color;
	float4 gray = (c.r+c.g+c.b)/3.0f;
	return lerp(c, gray, grayscale);
}

//******************************************************************************
// Zmienne
//******************************************************************************
cbuffer PsGlobals : register(b0)
{
	float4 skill;
	float power;
	float time;
};

Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse;

//******************************************************************************
struct VS_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
};

//******************************************************************************
// Vertex shader który nic nie robi
//******************************************************************************
void VsEmpty(VS_INPUT In, out VS_OUTPUT Out)
{
	Out.pos = float4(In.pos, 1);
	Out.tex = In.tex;
}

float4 PsEmpty(VS_OUTPUT In) : SV_TARGET
{
	return texDiffuse.Sample(samplerDiffuse, In.tex);
}

//******************************************************************************
// Przekszta³ca na szary obraz, parametry:
// float power(0..1) - 0 brak efektu, 1 ca³kowicie szary
//******************************************************************************
float4 PsMonochrome(VS_OUTPUT In) : SV_TARGET
{
	float4 base_color = texDiffuse.Sample(samplerDiffuse, In.tex);
	float4 color;
	color.a = base_color.a;
	color.rgb = (base_color.r+base_color.g+base_color.b)/3.0f;
	return lerp(base_color,color,power);
}

//******************************************************************************
// Efekt snu, parametry:
// float time - postêp efektu
// float power - si³a (0..1)
// float4 skill - gdzie:
//   0 - blur (2)
//   1 - sharpness (7)
//   2 - speed (0.1)
//   3 - range (0.4)
//******************************************************************************
float4 PsDream(VS_OUTPUT In) : SV_TARGET
{
	float2 tex = In.tex;
	float4 base_color = texDiffuse.Sample(samplerDiffuse, tex);
	tex.xy -= 0.5;
	tex.xy *= 1-(sin(time*skill[2])*skill[3]+skill[3])*0.5;
	tex.xy += 0.5;
	
	float4 color = texDiffuse.Sample(samplerDiffuse, tex.xy);

	color += texDiffuse.Sample(samplerDiffuse, tex.xy+0.001*skill[0]);
	color += texDiffuse.Sample(samplerDiffuse, tex.xy+0.003*skill[0]);
	color += texDiffuse.Sample(samplerDiffuse, tex.xy+0.005*skill[0]);
	color += texDiffuse.Sample(samplerDiffuse, tex.xy+0.007*skill[0]);
	color += texDiffuse.Sample(samplerDiffuse, tex.xy+0.009*skill[0]);
	color += texDiffuse.Sample(samplerDiffuse, tex.xy+0.011*skill[0]);

	color += texDiffuse.Sample(samplerDiffuse, tex.xy-0.001*skill[0]);
	color += texDiffuse.Sample(samplerDiffuse, tex.xy-0.003*skill[0]);
	color += texDiffuse.Sample(samplerDiffuse, tex.xy-0.005*skill[0]);
	color += texDiffuse.Sample(samplerDiffuse, tex.xy-0.007*skill[0]);
	color += texDiffuse.Sample(samplerDiffuse, tex.xy-0.009*skill[0]);
	color += texDiffuse.Sample(samplerDiffuse, tex.xy-0.011*skill[0]);

	color.rgb = (color.r+color.g+color.b)/3.0f;

	color /= skill[1];
	return lerp(base_color,color,power);
}

//******************************************************************************
// Efekt rozmycia, trzeba robiæ 2 razy, raz po x, raz po y, parametry:
// float power - si³a (0..1)
// float4 skill - gdzie:
// 	0 - odstêp po x
// 	1 - odstêp po y
//******************************************************************************
float4 PsBlurX(VS_OUTPUT In) : SV_TARGET
{
	float2 tex = In.tex;
	float4
	c  = texDiffuse.Sample(samplerDiffuse, float2(tex.x - 4.f*skill[0], tex.y)) * 0.05f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x - 3.f*skill[0], tex.y)) * 0.09f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x - 2.f*skill[0], tex.y)) * 0.12f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x - 1.f*skill[0], tex.y)) * 0.15f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x               , tex.y)) * 0.16f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x + 1.f*skill[0], tex.y)) * 0.15f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x + 2.f*skill[0], tex.y)) * 0.12f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x + 3.f*skill[0], tex.y)) * 0.09f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x + 4.f*skill[0], tex.y)) * 0.05f;
	return lerp(texDiffuse.Sample(samplerDiffuse, tex), c, power);
}

float4 PsBlurY(VS_OUTPUT In) : SV_TARGET
{
	float2 tex = In.tex;
	float4
	c  = texDiffuse.Sample(samplerDiffuse, float2(tex.x, tex.y - 4.f*skill[1])) * 0.05f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x, tex.y - 3.f*skill[1])) * 0.09f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x, tex.y - 2.f*skill[1])) * 0.12f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x, tex.y - 1.f*skill[1])) * 0.15f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x, tex.y               )) * 0.16f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x, tex.y + 1.f*skill[1])) * 0.15f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x, tex.y + 2.f*skill[1])) * 0.12f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x, tex.y + 3.f*skill[1])) * 0.09f;
	c += texDiffuse.Sample(samplerDiffuse, float2(tex.x, tex.y + 4.f*skill[1])) * 0.05f;
	return lerp(texDiffuse.Sample(samplerDiffuse, tex), c, power);
}

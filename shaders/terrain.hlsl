// Terrain shader

//------------------------------------------------------------------------------
// VERTEX SHADER CONSTANTS
cbuffer VsGlobals : register(b0)
{
	matrix matCombined;
	matrix matWorld;
};

//------------------------------------------------------------------------------
// PIXEL SHADER CONSTANTS
cbuffer PsGlobals : register(b0)
{
	float4 colorAmbient;
	float4 colorDiffuse;
	float3 lightDir;
	float4 fogColor;
	float4 fogParam; //(x=fogStart,y=fogEnd,z=(fogEnd-fogStart),a=?)
};

//------------------------------------------------------------------------------
// TEXTURES
Texture2D texBlend : register(t0);
Texture2D tex0 : register(t1);
Texture2D tex1 : register(t2);
Texture2D tex2 : register(t3);
Texture2D tex3 : register(t4);
Texture2D tex4 : register(t5);
SamplerState samplerBlend;
SamplerState sampler0;
SamplerState sampler1;
SamplerState sampler2;
SamplerState sampler3;
SamplerState sampler4;

//------------------------------------------------------------------------------
// VERTEX SHADER INPUT
struct TERRAIN_INPUT
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
	float2 texBlend : TEXCOORD1;
};

//------------------------------------------------------------------------------
// VERTEX SHADER OUTPUT
struct TERRAIN_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float2 texBlend : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float posViewZ : TEXCOORD3;
};

//------------------------------------------------------------------------------
// VERTEX SHADER
TERRAIN_OUTPUT VsMain(in TERRAIN_INPUT In)
{
	TERRAIN_OUTPUT Out;
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.tex = In.tex;
	Out.texBlend = In.texBlend;
	Out.normal = mul(float4(In.normal,1), matWorld).xyz;
	Out.posViewZ = Out.pos.w;
	return Out;
}

//------------------------------------------------------------------------------
// PIXEL SHADER
float4 PsMain(in TERRAIN_OUTPUT In) : SV_TARGET
{
	float4 a = texBlend.Sample(samplerBlend, In.texBlend);
	float4 c0 = tex0.Sample(sampler0, In.tex);
	float4 c1 = tex1.Sample(sampler1, In.tex);
	float4 c2 = tex2.Sample(sampler2, In.tex);
	float4 c3 = tex3.Sample(sampler3, In.tex);
	float4 c4 = tex4.Sample(sampler4, In.tex);
	
	float4 texColor = lerp(c0,c1,a.b);
	texColor = lerp(texColor,c2,a.g);
	texColor = lerp(texColor,c3,a.r);
	texColor = lerp(texColor,c4,a.a);
	
	float4 diffuse = colorDiffuse * saturate(dot(lightDir,In.normal));

	float4 new_texel = float4( texColor.xyz * saturate(colorAmbient +
		diffuse).xyz, texColor.w );

	float l = saturate((In.posViewZ-fogParam.x)/fogParam.z);
	new_texel = lerp(new_texel, fogColor, l);

	return new_texel;
}

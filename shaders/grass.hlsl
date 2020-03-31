cbuffer vs_globals : register(b0)
{
	matrix matViewProj;
};

cbuffer ps_globals : register(b0)
{
	// mg-a
	float4 fogColor;
	float4 fogParam; //(x=fogStart,y=fogEnd,z=(fogEnd-fogStart),a=?)
	// o¶wietlenie ambient
	float3 ambientColor;
};

Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse;

//------------------------------------------------------------------------------
// WEJ¦CIE DO VERTEX SHADERA
struct MESH_INPUT
{
  float3 pos : POSITION;
  float3 normal : NORMAL;
  float2 tex : TEXCOORD0;
  float4 mat1 : TEXCOORD1;
  float4 mat2 : TEXCOORD2;
  float4 mat3 : TEXCOORD3;
  float4 mat4 : TEXCOORD4;
};

//------------------------------------------------------------------------------
// WYJ¦CIE Z VERTEX SHADERA
struct MESH_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float posViewZ : TEXCOORD1;
};

//------------------------------------------------------------------------------
// VERTEX SHADER
void vs_grass(in MESH_INPUT In, out MESH_OUTPUT Out)
{
	float4x4 matWorld;
	matWorld._m00_m01_m02_m03 = In.mat1.xyzw;
	matWorld._m10_m11_m12_m13 = In.mat2.xyzw;
	matWorld._m20_m21_m22_m23 = In.mat3.xyzw;
	matWorld._m30_m31_m32_m33 = In.mat4.xyzw;
	Out.tex = In.tex;
	Out.pos = mul(float4(In.pos,1), mul(matWorld, matViewProj));
	Out.posViewZ = Out.pos.w;
}

//------------------------------------------------------------------------------
// PIXEL SHADER
float4 ps_grass(in MESH_OUTPUT In) : SV_TARGET
{
	float4 tex = texDiffuse.Sample(samplerDiffuse, In.tex);
	float fog = saturate((In.posViewZ-fogParam.x)/fogParam.z);
	return float4(lerp(tex.xyz * ambientColor, fogColor, fog), tex.w);
}

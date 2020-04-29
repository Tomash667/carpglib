#if defined(ANIMATED) && !defined(HAVE_WEIGHT)
#	error "Animation require weights!"
#endif

#if defined(POINT_LIGHT) && defined(DIR_LIGHT)
#	error "Mixed lighting not supported yet!"
#endif

#if defined(NORMAL_MAP) && !defined(HAVE_TANGENTS)
#	error "Normal mapping require binormals!"
#endif

cbuffer VsGlobals : register(b0)
{
	float3 cameraPos;
};

cbuffer VsLocals : register(b1)
{
	matrix matCombined;
	matrix matWorld;
	matrix matBones[32];
};

cbuffer PsGlobals : register(b0)
{
	float4 ambientColor;
	float4 lightColor;
	float3 lightDir;
	float4 fogColor;
	float4 fogParams;
};

struct Light
{
	float3 color;
	float3 pos;
	float range;
};

cbuffer PsLocals : register(b1)
{
	float4 tint;
	Light lights[3];
	float alphaTest;
};

cbuffer PsMaterial : register(b2)
{
	float3 specularColor;
	float specularHardness;
	float specularIntensity;
};

Texture2D texDiffuse : register(t0);
Texture2D texNormal : register(t1);
Texture2D texSpecular : register(t2);
SamplerState samplerDiffuse : register(s0);
SamplerState samplerNormal : register(s1);
SamplerState samplerSpecular : register(s2);

struct VsInput
{
    float3 pos : POSITION;
#ifdef HAVE_WEIGHT
	float weight : BLENDWEIGHT0;
	uint4 indices : BLENDINDICES0;
#endif
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
#ifdef HAVE_TANGENTS
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
#endif
};

struct VsOutput
{
    float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float3 viewDir : TEXCOORD2;
#ifdef POINT_LIGHT
	float3 posWorld : TEXCOORD3;
#endif
#ifdef FOG
	float posViewZ : TEXCOORD4;
#endif
#ifdef NORMAL_MAP
	float3 tangent : TEXCOORD5;
	float3 binormal : TEXCOORD6;
#endif
};

void VsMain(VsInput In, out VsOutput Out)
{
	// pos
#ifdef ANIMATED
	float3 pos = mul(float4(In.pos,1), matBones[In.indices[0]]).xyz * In.weight;
	pos += mul(float4(In.pos,1), matBones[In.indices[1]]).xyz * (1-In.weight);
	Out.pos = mul(float4(pos,1), matCombined);
#else
	float3 pos = In.pos;
	Out.pos = mul(float4(pos,1), matCombined);
#endif

	// normal
#ifdef ANIMATED
	float3 normal = mul(float4(In.normal,1), matBones[In.indices[0]]).xyz * In.weight;
	normal += mul(float4(In.normal,1), matBones[In.indices[1]]).xyz * (1-In.weight);
	Out.normal = mul(normal, (float3x3)matWorld).xyz;
#else
	Out.normal = mul(In.normal, (float3x3)matWorld).xyz;
#endif

	// tangent/binormal
#ifdef NORMAL_MAP
	Out.tangent = normalize(mul(In.tangent, (float3x3)matWorld).xyz);
	Out.binormal = normalize(mul(In.binormal, (float3x3)matWorld).xyz);
#endif
	
	// tex
	Out.tex = In.tex;
	
	// direction from camera to vertex for specular calculations
	Out.viewDir = normalize(cameraPos - mul(float4(pos,1), matWorld).xyz);
	
	// pos to world
#ifdef POINT_LIGHT
	Out.posWorld = mul(float4(pos,1), matWorld).xyz;
#endif
	
	// distance for fog
#ifdef FOG
	Out.posViewZ = Out.pos.w;
#endif
}

float4 PsMain(VsOutput In) : SV_TARGET
{
	float4 tex = texDiffuse.Sample(samplerDiffuse, In.tex);
	clip(tex.w - alphaTest);
	tex *= tint;
	float4 color = ambientColor;
	
#ifdef NORMAL_MAP
	float3 bump = texNormal.Sample(samplerNormal, In.tex).xyz * 2.f - 1.f;
	float3 normal = normalize(bump.x * In.tangent + (-bump.y) * In.binormal + bump.z * In.normal);
#else
	float3 normal = In.normal;
#endif

	float specInt;
#ifdef SPECULAR_MAP
	float4 specTex = texSpecular.Sample(samplerSpecular, In.tex);
	specInt = specTex.r + (1.f - specTex.a) * specularIntensity;
#else
	specInt = specularIntensity;
#endif
	
#ifdef DIR_LIGHT
	float specular = 0;
	float lightIntensity = saturate(dot(normal, lightDir));
	if(lightIntensity > 0.f)
	{
		color = saturate(color + (lightColor * lightIntensity));
		
		float3 reflection = normalize(2 * lightIntensity * normal - lightDir);
		specular = pow(saturate(dot(reflection, In.viewDir)), specularHardness) * specInt;
	}
	tex.xyz = saturate((tex.xyz * color.xyz) + specularColor * specular);
#elif defined(POINT_LIGHT)
	float specular = 0;
	for(int i=0; i<3; ++i)
	{
		float3 lightVec = normalize(lights[i].pos - In.posWorld);
		float dist = distance(lights[i].pos, In.posWorld);
		float falloff = clamp((1 - (dist / lights[i].range)), 0, 1);
		float lightIntensity = clamp(dot(lightVec, normal),0,1) * falloff;
		if(lightIntensity > 0)
		{
			color.xyz += lightIntensity * lights[i].color;
			float3 reflection = normalize(2 * lightIntensity * normal - lightVec);
			specular += pow(saturate(dot(reflection, normalize(In.viewDir))), specularHardness) * specInt * falloff;
		}
	}
	specular = saturate(specular);
	tex.xyz = saturate((tex.xyz * saturate(color.xyz)) + specularColor * specular);
#endif
	
#ifdef FOG
	float fog = saturate((In.posViewZ - fogParams.x) / fogParams.z);
	return float4(lerp(tex.xyz, fogColor.xyz, fog), tex.w);
#else
	return tex;
#endif
}

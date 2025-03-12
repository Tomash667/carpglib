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
    matrix matLightViewProj;
	float3 cameraPos;
    float3 lightPosGlobal;
};

cbuffer VsLocals : register(b1)
{
	matrix matCombined;
	matrix matWorld;
	matrix matBones[64];
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
Texture2D texDepth : register(t3);
SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);
// TODO: sampler2

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
    float4 lightViewPosition : TEXCOORD7;
    float3 lightPos : TEXCOORD8;
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
	
	// shadow map
    float4 worldPos = mul(float4(pos, 1), matWorld);
    Out.lightViewPosition = mul(worldPos, matLightViewProj);
    Out.lightPos = normalize(lightPosGlobal.xyz - worldPos.xyz);
}

float4 PsMain(VsOutput In) : SV_TARGET
{
	float4 tex = texDiffuse.Sample(sampler0, In.tex);
	clip(tex.w - alphaTest);
	tex *= tint;
	float4 color = ambientColor;
	
#ifdef NORMAL_MAP
	float3 bump = texNormal.Sample(sampler0, In.tex).xyz * 2.f - 1.f;
	float3 normal = normalize(bump.x * In.tangent + (-bump.y) * In.binormal + bump.z * In.normal);
#else
	float3 normal = In.normal;
#endif

	float specInt;
#ifdef SPECULAR_MAP
	float4 specTex = texSpecular.Sample(sampler0, In.tex);
	specInt = specTex.r + (1.f - specTex.a) * specularIntensity;
#else
	specInt = specularIntensity;
#endif
	
	/*
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
	*/
	
    //color = float4(0.5f, 0.5f, 0.5f, 1);
    //color = float4(0, 0, 0, 1);
	
    float2 projectTexCoord;
	
	// Calculate the projected texture coordinates.
    projectTexCoord.x = In.lightViewPosition.x / In.lightViewPosition.w / 2.0f + 0.5f;
    projectTexCoord.y = -In.lightViewPosition.y / In.lightViewPosition.w / 2.0f + 0.5f;
	
    // Determine if the projected coordinates are in the 0 to 1 range.  If it is then this pixel is inside the projected view port.
    if ((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))
    {
        //return float4(1, 0, 0, 1);
        // Sample the shadow map depth value from the depth texture using the sampler at the projected texture coordinate location.
        float depthValue = texDepth.Sample(sampler1, projectTexCoord).r;
        //return float4(depthValue, depthValue, depthValue, 1);

        // Calculate the depth of the light.
        float lightDepthValue = In.lightViewPosition.z / In.lightViewPosition.w;

        // Subtract the bias from the lightDepthValue.
        //lightDepthValue = lightDepthValue - 0.0022f; // bias
        lightDepthValue = lightDepthValue - 0.003f; // bias

         // Compare the depth of the shadow map value and the depth of the light to determine whether to shadow or to light this pixel.
        // If the light is in front of the object then light the pixel, if not then shadow this pixel since an object (occluder) is casting a shadow on it.
        if (lightDepthValue < depthValue)
        {
            //return float4(1, 0, 0, 1);
            // Calculate the amount of light on this pixel.
            float lightIntensity = saturate(dot(In.normal, In.lightPos));
            //color = float4(lightIntensity, lightIntensity, lightIntensity, 1);
            //return float4(lightIntensity, lightIntensity, lightIntensity, 1);

            if (lightIntensity > 0.0f)
            {
                // Determine the final diffuse color based on the diffuse color and the amount of light intensity.
                //color += (diffuseColor * lightIntensity);
                color += (float4(1, 1, 1, 1) * lightIntensity);

                // Saturate the final light color.
                color = saturate(color);
            }
            //color = float4(1, 1, 1, 1);
        }

    }
	
	// Combine the light and texture color.
    color = color * tex;

    return color;
}

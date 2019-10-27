float2 size;
texture tex0;

sampler sampler0 = sampler_state
{
	Texture = <tex0>;
	MipFilter = None;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Clamp;
	AddressV = Clamp;
};

struct VERTEX_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR;
};

struct VERTEX_OUTPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR;
};

void vs_main(in VERTEX_INPUT In, out VERTEX_OUTPUT Out)
{
	// fix half pixel problem
	Out.pos.x = ((In.pos.x - 0.5f) / (size.x * 0.5f)) - 1.0f;
	Out.pos.y = -(((In.pos.y - 0.5f) / (size.y * 0.5f)) - 1.0f);
	Out.pos.z = 0.f;
	Out.pos.w = 1.f;
	Out.tex = In.tex;
	Out.color = In.color;
}

float4 ps_tex(in VERTEX_OUTPUT In) : COLOR0
{
	float4 c = tex2D(sampler0, In.tex);
	return c * In.color;
}

float4 ps_color(in VERTEX_OUTPUT In) : COLOR0
{
	return In.color;
}

float4 ps_grayscale(in VERTEX_OUTPUT In) : COLOR0
{
	float4 c = tex2D(sampler0, In.tex) * In.color;
	c.rgb = (c.r+c.g+c.b)/3.0f;
	return c;
}

technique techTex
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_main();
		PixelShader = compile PS_VERSION ps_tex();
	}
}

technique techColor
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_main();
		PixelShader = compile PS_VERSION ps_color();
	}
}

technique techGrayscale
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_main();
		PixelShader = compile PS_VERSION ps_grayscale();
	}
}

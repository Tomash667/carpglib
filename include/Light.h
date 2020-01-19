#pragma once

//-----------------------------------------------------------------------------
struct Light
{
	Vec3 pos;
	float range;
	Vec4 color;
};

//-----------------------------------------------------------------------------
struct Lights
{
	static constexpr int COUNT = 3;

	Light ld[COUNT];
};

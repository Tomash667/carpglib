#pragma once

//-----------------------------------------------------------------------------
struct Light
{
	Vec4 color;
	Vec3 pos;
	float range;

	static const Light EMPTY;
};

inline const Light Light::EMPTY = { Vec4::Zero, Vec3::Zero, 1 };

//-----------------------------------------------------------------------------
struct Lights
{
	static constexpr int COUNT = 3;

	Light ld[COUNT];
};

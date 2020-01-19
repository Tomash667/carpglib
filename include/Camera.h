#pragma once

//-----------------------------------------------------------------------------
struct Camera
{
	Camera() : znear(0.1f), zfar(50.f) {}

	Matrix mat_view_proj;
	Matrix mat_view_inv;
	Vec3 from, to;
	float znear, zfar;
};

#pragma once

//-----------------------------------------------------------------------------
struct Camera
{
	Camera() : fov(PI / 4), znear(0.1f), zfar(50.f) {}
	void Setup();

	Matrix mat_view_proj;
	Matrix mat_view_inv;
	Vec3 from, to;
	float fov, znear, zfar;
};

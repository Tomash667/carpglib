#pragma once

//-----------------------------------------------------------------------------
struct Camera
{
	Camera() : from(-2, 2, -2), to(0, 0, 0), znear(0.1f), zfar(50.f) {}
	void UpdateMatrix();
	Vec3 GetDir() const { return (to - from).Normalized(); }

	Matrix mat_view_proj;
	Matrix mat_view_inv;
	Vec3 from, to;
	float znear, zfar;
};

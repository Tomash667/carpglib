#pragma once

//-----------------------------------------------------------------------------
struct Camera
{
	Camera() : fov(PI / 4), znear(0.1f), zfar(50.f) {}
	void UpdateMatrix();

	Matrix matViewProj;
	Matrix matViewInv;
	Vec3 from, to;
	float fov, znear, zfar;
};

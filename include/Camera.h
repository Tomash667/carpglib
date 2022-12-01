#pragma once

//-----------------------------------------------------------------------------
struct Camera
{
	Camera() : znear(0.1f), zfar(50.f) {}

	Matrix matViewProj;
	Matrix matViewInv;
	Vec3 from, to;
	float znear, zfar;
};

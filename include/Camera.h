#pragma once

//-----------------------------------------------------------------------------
struct Camera
{
	Camera() : up(Vec3::Up), aspect(0), fov(PI / 4), znear(0.1f), zfar(50.f) {}
	void UpdateMatrix();
	Vec3 GetDir() const { return (to - from).Normalized(); }

	Matrix matViewProj;
	Matrix matViewInv;
	Vec3 from, to, up;
	float aspect, fov, znear, zfar;
};

#pragma once

//-----------------------------------------------------------------------------
struct CameraBase
{
	Matrix mat_view_proj;
	Matrix mat_view_inv;
	Vec3 from, to;
};

#include "EnginePch.h"
#include "EngineCore.h"
#include "Camera.h"
#include "Engine.h"

//=================================================================================================
void Camera::Setup()
{
	const Matrix mat_view = Matrix::CreateLookAt(from, to);
	const Matrix mat_proj = Matrix::CreatePerspectiveFieldOfView(fov, app::engine->GetWindowAspect(), znear, zfar);
	mat_view_proj = mat_view * mat_proj;
	mat_view_inv = mat_view.Inverse();
}

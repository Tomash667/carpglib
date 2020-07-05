#include "Pch.h"
#include "Camera.h"

#include <Engine.h>

void Camera::UpdateMatrix()
{
	Matrix matView = Matrix::CreateLookAt(from, to);
	Matrix matProj = Matrix::CreatePerspectiveFieldOfView(PI / 4, app::engine->GetWindowAspect(), znear, zfar);
	mat_view_proj = matView * matProj;
	mat_view_inv = matView.Inverse();
}

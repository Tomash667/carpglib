#include "Pch.h"
#include "Camera.h"

#include <Engine.h>

//=================================================================================================
void Camera::UpdateMatrix()
{
	Matrix matView = Matrix::CreateLookAt(from, to);
	Matrix matProj = Matrix::CreatePerspectiveFieldOfView(fov, app::engine->GetWindowAspect(), znear, zfar);
	matViewProj = matView * matProj;
	matViewInv = matView.Inverse();
}

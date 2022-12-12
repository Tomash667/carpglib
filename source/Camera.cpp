#include "Pch.h"
#include "Camera.h"

#include <Engine.h>

//=================================================================================================
void Camera::UpdateMatrix()
{
	const float usedAspect = aspect > 0 ? aspect : app::engine->GetWindowAspect();
	Matrix matView = Matrix::CreateLookAt(from, to, up);
	Matrix matProj = Matrix::CreatePerspectiveFieldOfView(fov, usedAspect, znear, zfar);
	matViewProj = matView * matProj;
	matViewInv = matView.Inverse();
}

#pragma once

//-----------------------------------------------------------------------------
struct CustomMesh
{
	virtual void Draw(Scene& scene, Camera& camera) = 0;
};

#include "Pch.h"
#include "VertexData.h"

bool VertexData::RayToMesh(const Vec3& rayPos, const Vec3& rayDir, const Vec3& objPos, float objRot, float& outDist) const
{
	// najpierw sprawdŸ kolizje promienia ze sfer¹ otaczaj¹c¹ model
	if(!RayToSphere(rayPos, rayDir, objPos, radius, outDist))
		return false;

	// przekszta³æ promieñ o pozycjê i obrót modelu
	Matrix m = (Matrix::RotationY(objRot) * Matrix::Translation(objPos)).Inverse();
	Vec3 rayPosT = Vec3::Transform(rayPos, m),
		ray_dir_t = Vec3::TransformNormal(rayDir, m);

	// szukaj kolizji
	outDist = 1.01f;
	float dist;
	bool hit = false;

	for(vector<Face>::const_iterator it = faces.begin(), end = faces.end(); it != end; ++it)
	{
		if(RayToTriangle(rayPosT, ray_dir_t, verts[it->idx[0]], verts[it->idx[1]], verts[it->idx[2]], dist) && dist < outDist && dist >= 0.f)
		{
			hit = true;
			outDist = dist;
		}
	}

	return hit;
}

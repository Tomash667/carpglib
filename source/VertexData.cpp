#include "Pch.h"
#include "VertexData.h"

#include "VertexDeclaration.h"

bool VertexData::RayToMesh(const Vec3& rayPos, const Vec3& rayDir, const Vec3& objPos, float objRot, float& outDist) const
{
	assert(vertexDecl == VDI_POS);

	// najpierw sprawdŸ kolizje promienia ze sfer¹ otaczaj¹c¹ model
	if(!RayToSphere(rayPos, rayDir, objPos, radius, outDist))
		return false;

	// przekszta³æ promieñ o pozycjê i obrót modelu
	Matrix m = (Matrix::RotationY(objRot) * Matrix::Translation(objPos)).Inverse();
	Vec3 rayPosT = Vec3::Transform(rayPos, m),
		rayDirT = Vec3::TransformNormal(rayDir, m);

	// szukaj kolizji
	outDist = 1.01f;
	float dist;
	bool hit = false;
	const vector<Vec3>& v = *reinterpret_cast<const vector<Vec3>*>(&verts);

	for(vector<Face>::const_iterator it = faces.begin(), end = faces.end(); it != end; ++it)
	{
		if(RayToTriangle(rayPosT, rayDirT, v[it->idx[0]], v[it->idx[1]], v[it->idx[2]], dist) && dist < outDist && dist >= 0.f)
		{
			hit = true;
			outDist = dist;
		}
	}

	return hit;
}

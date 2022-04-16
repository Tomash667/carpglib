#include "Pch.h"
#include "VertexData.h"

bool VertexData::RayToMesh(const Vec3& ray_pos, const Vec3& ray_dir, const Vec3& obj_pos, float obj_rot, float& out_dist) const
{
	assert(vertex_size == sizeof(Vec3));

	// najpierw sprawd� kolizje promienia ze sfer� otaczaj�c� model
	if(!RayToSphere(ray_pos, ray_dir, obj_pos, radius, out_dist))
		return false;

	// przekszta�� promie� o pozycj� i obr�t modelu
	Matrix m = (Matrix::RotationY(obj_rot) * Matrix::Translation(obj_pos)).Inverse();
	Vec3 ray_pos_t = Vec3::Transform(ray_pos, m),
		ray_dir_t = Vec3::TransformNormal(ray_dir, m);

	// szukaj kolizji
	out_dist = 1.01f;
	float dist;
	bool hit = false;
	const vector<Vec3>& v = *reinterpret_cast<const vector<Vec3>*>(&verts);

	for(vector<Face>::const_iterator it = faces.begin(), end = faces.end(); it != end; ++it)
	{
		if(RayToTriangle(ray_pos_t, ray_dir_t, v[it->idx[0]], v[it->idx[1]], v[it->idx[2]], dist) && dist < out_dist && dist >= 0.f)
		{
			hit = true;
			out_dist = dist;
		}
	}

	return hit;
}

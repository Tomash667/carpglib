#include "Pch.h"

const string Guid::EmptyString = "00000000-0000-0000-0000-000000000000";

void MurmurHash3_x86_32(const void* key, int len, uint32_t seed, void* out);

//=================================================================================================
// Random number generator
//=================================================================================================
std::minstd_rand internal::rng;

uint RandVal()
{
	// ugly hack
	static_assert(sizeof(internal::rng) == sizeof(uint), "Failed to get random seed, implementation changed.");
	uint seed = *(uint*)&internal::rng;
	return seed;
}

void Srand()
{
	std::random_device rdev;
	internal::rng.seed(rdev());
}

//=================================================================================================
// Return angle between two points
//=================================================================================================
float Angle(float x1, float y1, float x2, float y2)
{
	float x = x2 - x1;
	float y = y2 - y1;

	if(x == 0)
	{
		if(y == 0)
			return 0;
		else if(y > 0)
			return PI / 2.f;
		else
			return PI * 3.f / 2.f;
	}
	else if(y == 0)
	{
		if(x > 0)
			return 0;
		else
			return PI;
	}
	else
	{
		if(x < 0)
			return atan(y / x) + PI;
		else if(y < 0)
			return atan(y / x) + (2 * PI);
		else
			return atan(y / x);
	}
}

//=================================================================================================
// Ray and box test
// If there is no collision returns false.
// If there is collision returns true and outT is multiple of rayDir.
// If there is collision with back of ray returns true and outT is negative.
// If rayPos is inside box it returns true and outT is 0.
//=================================================================================================
bool RayToBox(const Vec3& rayPos, const Vec3& rayDir, const Box &box, float* outT)
{
	bool inside = true;

	float xt;
	if(rayPos.x < box.v1.x)
	{
		xt = box.v1.x - rayPos.x;
		xt /= rayDir.x;
		inside = false;
	}
	else if(rayPos.x > box.v2.x)
	{
		xt = box.v2.x - rayPos.x;
		xt /= rayDir.x;
		inside = false;
	}
	else
		xt = -1.0f;

	float yt;
	if(rayPos.y < box.v1.y)
	{
		yt = box.v1.y - rayPos.y;
		yt /= rayDir.y;
		inside = false;
	}
	else if(rayPos.y > box.v2.y)
	{
		yt = box.v2.y - rayPos.y;
		yt /= rayDir.y;
		inside = false;
	}
	else
		yt = -1.0f;

	float zt;
	if(rayPos.z < box.v1.z)
	{
		zt = box.v1.z - rayPos.z;
		zt /= rayDir.z;
		inside = false;
	}
	else if(rayPos.z > box.v2.z)
	{
		zt = box.v2.z - rayPos.z;
		zt /= rayDir.z;
		inside = false;
	}
	else
		zt = -1.0f;

	if(inside)
	{
		*outT = 0.0f;
		return true;
	}

	// Select the farthest plane - this is the plane of intersection
	int plane = 0;
	float t = xt;
	if(yt > t)
	{
		plane = 1;
		t = yt;
	}

	if(zt > t)
	{
		plane = 2;
		t = zt;
	}

	// Check if the point of intersection lays within the box face
	switch(plane)
	{
	case 0: // ray intersects with yz plane
		{
			float y = rayPos.y + rayDir.y * t;
			if(y < box.v1.y || y > box.v2.y)
				return false;
			float z = rayPos.z + rayDir.z * t;
			if(z < box.v1.z || z > box.v2.z)
				return false;
		}
		break;
	case 1: // ray intersects with xz plane
		{
			float x = rayPos.x + rayDir.x * t;
			if(x < box.v1.x || x > box.v2.x)
				return false;
			float z = rayPos.z + rayDir.z * t;
			if(z < box.v1.z || z > box.v2.z)
				return false;
		}
		break;
	default:
	case 2: // ray intersects with xy plane
		{
			float x = rayPos.x + rayDir.x * t;
			if(x < box.v1.x || x > box.v2.x)
				return false;
			float y = rayPos.y + rayDir.y * t;
			if(y < box.v1.y || y > box.v2.y)
				return false;
		}
		break;
	}

	*outT = t;
	return true;
}

//=================================================================================================
// Returns shortest direction to rotate with two angles
//=================================================================================================
float ShortestArc(float a, float b)
{
	if(fabs(b - a) < PI)
		return b - a;
	if(b > a)
		return b - a - PI * 2.0f;
	return b - a + PI * 2.0f;
}

//=================================================================================================
// Lerp interpolation of angles
//=================================================================================================
void LerpAngle(float& angle, float from, float to, float t)
{
	if(to > angle)
	{
		while(to - angle > PI)
			to -= PI * 2;
	}
	else
	{
		while(to - angle < -PI)
			to += PI * 2;
	}

	angle = from + t * (to - from);
}

int AdjustAngle(float& angle, float expected, float maxDif)
{
	if(angle == expected)
		return 0;

	float dif = AngleDif(angle, expected);
	if(dif <= maxDif)
	{
		angle = expected;
		return 0;
	}
	else
	{
		float dir = ShortestArc(angle, expected);
		angle = Clip(angle + dir * maxDif);
		return dir > 0 ? 1 : -1;
	}
}

bool CircleToRectangle(float circlex, float circley, float radius, float rectx, float recty, float w, float h)
{
	//
	//        /|\ -h
	//         |
	//         |
	//  -w <--(x,y)--> w
	//         |
	//         |
	//        \|/  h
	float distX = abs(circlex - rectx);
	float distY = abs(circley - recty);

	if((distX > (w + radius)) || (distY > (h + radius)))
		return false;

	if((distX <= w) || (distY <= h))
		return true;

	float dx = distX - w;
	float dy = distY - h;

	return (dx * dx + dy * dy) <= (radius * radius);
}

bool Vec3::Parse(cstring str)
{
	return sscanf_s(str, "%g; %g; %g", &x, &y, &z) == 3;
}

cstring Vec3::ToString() const
{
	return Format("%g; %g; %g", x, y, z);
}

bool Box2d::IsFullyInside(const Vec2& v, float r) const
{
	return v.x - r >= v1.x
		&& v.x + r <= v2.x
		&& v.y - r >= v1.y
		&& v.y + r <= v2.y;
}

bool Plane::Intersect3Planes(const Plane& P1, const Plane& P2, const Plane& P3, Vec3& OutP)
{
	float fDet;
	float MN[9] = { P1.x, P1.y, P1.z, P2.x, P2.y, P2.z, P3.x, P3.y, P3.z };
	float IMN[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	float MD[3] = { -P1.w, -P2.w , -P3.w };

	IMN[0] = MN[4] * MN[8] - MN[5] * MN[7];
	IMN[3] = -(MN[3] * MN[8] - MN[5] * MN[6]);
	IMN[6] = MN[3] * MN[7] - MN[4] * MN[6];

	fDet = MN[0] * IMN[0] + MN[1] * IMN[3] + MN[2] * IMN[6];

	if(AlmostZero(fDet))
		return false;

	IMN[1] = -(MN[1] * MN[8] - MN[2] * MN[7]);
	IMN[4] = MN[0] * MN[8] - MN[2] * MN[6];
	IMN[7] = -(MN[0] * MN[7] - MN[1] * MN[6]);
	IMN[2] = MN[1] * MN[5] - MN[2] * MN[4];
	IMN[5] = -(MN[0] * MN[5] - MN[2] * MN[3]);
	IMN[8] = MN[0] * MN[4] - MN[1] * MN[3];

	fDet = 1.0f / fDet;

	IMN[0] *= fDet;
	IMN[1] *= fDet;
	IMN[2] *= fDet;
	IMN[3] *= fDet;
	IMN[4] *= fDet;
	IMN[5] *= fDet;
	IMN[6] *= fDet;
	IMN[7] *= fDet;
	IMN[8] *= fDet;

	OutP.x = IMN[0] * MD[0] + IMN[1] * MD[1] + IMN[2] * MD[2];
	OutP.y = IMN[3] * MD[0] + IMN[4] * MD[1] + IMN[5] * MD[2];
	OutP.z = IMN[6] * MD[0] + IMN[7] * MD[1] + IMN[8] * MD[2];

	return true;
}

void FrustumPlanes::Set(const Matrix& worldViewProj)
{
	// Left clipping plane
	planes[0].x = worldViewProj._14 + worldViewProj._11;
	planes[0].y = worldViewProj._24 + worldViewProj._21;
	planes[0].z = worldViewProj._34 + worldViewProj._31;
	planes[0].w = worldViewProj._44 + worldViewProj._41;
	planes[0].Normalize();

	// Right clipping plane
	planes[1].x = worldViewProj._14 - worldViewProj._11;
	planes[1].y = worldViewProj._24 - worldViewProj._21;
	planes[1].z = worldViewProj._34 - worldViewProj._31;
	planes[1].w = worldViewProj._44 - worldViewProj._41;
	planes[1].Normalize();

	// Top clipping plane
	planes[2].x = worldViewProj._14 - worldViewProj._12;
	planes[2].y = worldViewProj._24 - worldViewProj._22;
	planes[2].z = worldViewProj._34 - worldViewProj._32;
	planes[2].w = worldViewProj._44 - worldViewProj._42;
	planes[2].Normalize();

	// Bottom clipping plane
	planes[3].x = worldViewProj._14 + worldViewProj._12;
	planes[3].y = worldViewProj._24 + worldViewProj._22;
	planes[3].z = worldViewProj._34 + worldViewProj._32;
	planes[3].w = worldViewProj._44 + worldViewProj._42;
	planes[3].Normalize();

	// Near clipping plane
	planes[4].x = worldViewProj._13;
	planes[4].y = worldViewProj._23;
	planes[4].z = worldViewProj._33;
	planes[4].w = worldViewProj._43;
	planes[4].Normalize();

	// Far clipping plane
	planes[5].x = worldViewProj._14 - worldViewProj._13;
	planes[5].y = worldViewProj._24 - worldViewProj._23;
	planes[5].z = worldViewProj._34 - worldViewProj._33;
	planes[5].w = worldViewProj._44 - worldViewProj._43;
	planes[5].Normalize();
}

void FrustumPlanes::Set(const array<Vec3, 8>& pts)
{
	// Left clipping plane
	planes[0] = Plane(
		pts[NEAR_LEFT_BOTTOM],
		pts[NEAR_LEFT_TOP],
		pts[FAR_LEFT_TOP]);

	// Right clipping plane
	planes[1] = Plane(
		pts[FAR_RIGHT_BOTTOM],
		pts[FAR_RIGHT_TOP],
		pts[NEAR_RIGHT_TOP]);

	// Top clipping plane
	planes[2] = Plane(
		pts[FAR_LEFT_TOP],
		pts[NEAR_LEFT_TOP],
		pts[NEAR_RIGHT_TOP]);

	// Bottom clipping plane
	planes[3] = Plane(
		pts[NEAR_LEFT_BOTTOM],
		pts[FAR_LEFT_BOTTOM],
		pts[FAR_RIGHT_BOTTOM]);

	// Near clipping plane
	planes[4] = Plane(
		pts[NEAR_RIGHT_BOTTOM],
		pts[NEAR_RIGHT_TOP],
		pts[NEAR_LEFT_TOP]);

	// Far clipping plane
	planes[5] = Plane(
		pts[FAR_LEFT_BOTTOM],
		pts[FAR_LEFT_TOP],
		pts[FAR_RIGHT_TOP]);
}

void FrustumPlanes::Construct(const Vec3& from, float rot, float dist, const Vec2& width, const Vec2& height)
{
	array<Vec3, 8> pts;
	Matrix mat = Matrix::RotationY(rot + PI) * Matrix::Translation(from);

	pts[NEAR_LEFT_BOTTOM] = Vec3::Transform(Vec3(-width.x / 2, -height.x / 2, 0), mat);
	pts[NEAR_RIGHT_BOTTOM] = Vec3::Transform(Vec3(width.x / 2, -height.x / 2, 0), mat);
	pts[NEAR_LEFT_TOP] = Vec3::Transform(Vec3(-width.x / 2, height.x / 2, 0), mat);
	pts[NEAR_RIGHT_TOP] = Vec3::Transform(Vec3(width.x / 2, height.x / 2, 0), mat);
	pts[FAR_LEFT_BOTTOM] = Vec3::Transform(Vec3(-width.y / 2, -height.y / 2, dist), mat);
	pts[FAR_RIGHT_BOTTOM] = Vec3::Transform(Vec3(width.y / 2, -height.y / 2, dist), mat);
	pts[FAR_LEFT_TOP] = Vec3::Transform(Vec3(-width.y / 2, height.y / 2, dist), mat);
	pts[FAR_RIGHT_TOP] = Vec3::Transform(Vec3(width.y / 2, height.y / 2, dist), mat);

	Set(pts);
}

void FrustumPlanes::GetPoints(array<Vec3, 8>& points) const
{
	Plane::Intersect3Planes(planes[4], planes[0], planes[3], points[0]);
	Plane::Intersect3Planes(planes[4], planes[1], planes[3], points[1]);
	Plane::Intersect3Planes(planes[4], planes[0], planes[2], points[2]);
	Plane::Intersect3Planes(planes[4], planes[1], planes[2], points[3]);
	Plane::Intersect3Planes(planes[5], planes[0], planes[3], points[4]);
	Plane::Intersect3Planes(planes[5], planes[1], planes[3], points[5]);
	Plane::Intersect3Planes(planes[5], planes[0], planes[2], points[6]);
	Plane::Intersect3Planes(planes[5], planes[1], planes[2], points[7]);
}

void FrustumPlanes::GetPoints(const Matrix& worldViewProj, array<Vec3, 8>& points)
{
	Matrix worldViewProjInv;
	worldViewProj.Inverse(worldViewProjInv);

	Vec3 P[] = {
		Vec3(-1.f, -1.f, 0.f), Vec3(+1.f, -1.f, 0.f),
		Vec3(-1.f, +1.f, 0.f), Vec3(+1.f, +1.f, 0.f),
		Vec3(-1.f, -1.f, 1.f), Vec3(+1.f, -1.f, 1.f),
		Vec3(-1.f, +1.f, 1.f), Vec3(+1.f, +1.f, 1.f) };

	for(int i = 0; i < 8; ++i)
		points[i] = Vec3::Transform(P[i], worldViewProjInv);
}

Box2d FrustumPlanes::GetBox2d() const
{
	array<Vec3, 8> pts;
	GetPoints(pts);
	float minx, minz, maxx, maxz;
	minx = maxx = pts[0].x;
	minz = maxz = pts[0].z;
	for(int i = 1; i < 8; ++i)
	{
		const Vec3& pt = pts[i];
		if(pt.x < minx)
			minx = pt.x;
		else if(pt.x > maxx)
			maxx = pt.x;
		if(pt.z < minz)
			minz = pt.z;
		else if(pt.z > maxz)
			maxz = pt.z;
	}
	return Box2d(minx, minz, maxx, maxz);
}

bool FrustumPlanes::PointInFrustum(const Vec3 &p) const
{
	for(int i = 0; i < 6; ++i)
	{
		if(planes[i].DotCoordinate(p) <= 0.f)
			return false;
	}

	return true;
}

bool FrustumPlanes::BoxToFrustum(const Box& box) const
{
	Vec3 vmin;

	for(int i = 0; i < 6; i++)
	{
		if(planes[i].x <= 0.0f)
			vmin.x = box.v1.x;
		else
			vmin.x = box.v2.x;

		if(planes[i].y <= 0.0f)
			vmin.y = box.v1.y;
		else
			vmin.y = box.v2.y;

		if(planes[i].z <= 0.0f)
			vmin.z = box.v1.z;
		else
			vmin.z = box.v2.z;

		if(planes[i].DotCoordinate(vmin) < 0.0f)
			return false;
	}

	return true;
}

bool FrustumPlanes::BoxToFrustum(const Box2d& box) const
{
	Vec3 vmin;

	for(int i = 0; i < 6; i++)
	{
		if(planes[i].x <= 0.0f)
			vmin.x = box.v1.x;
		else
			vmin.x = box.v2.x;

		if(planes[i].y <= 0.0f)
			vmin.y = -999.f;
		else
			vmin.y = 999.f;

		if(planes[i].z <= 0.0f)
			vmin.z = box.v1.y;
		else
			vmin.z = box.v2.y;

		if(planes[i].DotCoordinate(vmin) < 0.0f)
			return false;
	}

	return true;
}

bool FrustumPlanes::BoxInFrustum(const Box& box) const
{
	if(!PointInFrustum(box.v1)) return false;
	if(!PointInFrustum(box.v2)) return false;
	if(!PointInFrustum(Vec3(box.v2.x, box.v1.y, box.v1.z))) return false;
	if(!PointInFrustum(Vec3(box.v1.x, box.v2.y, box.v1.z))) return false;
	if(!PointInFrustum(Vec3(box.v2.x, box.v2.y, box.v1.z))) return false;
	if(!PointInFrustum(Vec3(box.v1.x, box.v1.y, box.v2.z))) return false;
	if(!PointInFrustum(Vec3(box.v2.x, box.v1.y, box.v2.z))) return false;
	if(!PointInFrustum(Vec3(box.v1.x, box.v2.y, box.v2.z))) return false;

	return true;
}

bool FrustumPlanes::SphereToFrustum(const Vec3& sphereCenter, float sphereRadius) const
{
	sphereRadius = -sphereRadius;

	for(int i = 0; i < 6; ++i)
	{
		if(planes[i].DotCoordinate(sphereCenter) <= sphereRadius)
			return false;
	}

	return true;
}

bool FrustumPlanes::SphereInFrustum(const Vec3& sphereCenter, float sphereRadius) const
{
	for(int i = 0; i < 6; ++i)
	{
		if(planes[i].DotCoordinate(sphereCenter) < sphereRadius)
			return false;
	}

	return true;
}

bool RayToPlane(const Vec3& rayPos, const Vec3& rayDir, const Plane& plane, float* outT)
{
	float VD = plane.x * rayDir.x + plane.y * rayDir.y + plane.z * rayDir.z;
	if(VD == 0.0f)
		return false;

	if(outT)
		*outT = -(plane.x * rayPos.x + plane.y * rayPos.y + plane.z * rayPos.z + plane.w) / VD;

	return true;
}

// https://www.shadertoy.com/view/XtlBDs
// 0-b-3
// |\
// a c
// |  \
// 1   2
// result: 0 - no intersection, 1 - intersection, -1 - backface intersection
int RayToQuad(const Vec3& rayPos, const Vec3& rayDir, const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, float* outT)
{
	// lets make v0 the origin
	Vec3 a = v1 - v0;
	Vec3 b = v3 - v0;
	Vec3 c = v2 - v0;
	Vec3 p = rayPos - v0;

	// intersect plane
	Vec3 nor = a.Cross(b);
	float t = -p.Dot(nor) / rayDir.Dot(nor);
	if(t < 0.0f)
		return 0;

	// intersection point
	Vec3 pos = p + t * rayDir;

	// select projection plane
	Vec3 mor = Vec3(abs(nor.x), abs(nor.y), abs(nor.z));
	int id = (mor.x > mor.y && mor.x > mor.z) ? 0 :
		(mor.y > mor.z) ? 1 :
		2;

	const int lut[4]{ 1, 2, 0, 1 };
	int idu = lut[id];
	int idv = lut[id + 1];

	// project to 2D
	Vec2 kp = Vec2(pos[idu], pos[idv]);
	Vec2 ka = Vec2(a[idu], a[idv]);
	Vec2 kb = Vec2(b[idu], b[idv]);
	Vec2 kc = Vec2(c[idu], c[idv]);

	// find barycentric coords of the quadrilateral
	Vec2 kg = kc - kb - ka;

	float k0 = kp.Cross2d(kb);
	float k2 = (kc - kb).Cross2d(ka);        // float k2 = cross2d( kg, ka );
	float k1 = kp.Cross2d(kg) - nor[id]; // float k1 = cross2d( kb, ka ) + cross2d( kp, kg );

	// if edges are parallel, this is a linear equation
	float u, v;
	if(abs(k2) < 0.00001f)
	{
		v = -k0 / k1;
		u = kp.Cross2d(ka) / k1;
	}
	else
	{
		// otherwise, it's a quadratic
		float w = k1 * k1 - 4.0f * k0 * k2;
		if(w < 0.0f)
			return 0;
		w = sqrt(w);

		float ik2 = 1.0f / (2.0f * k2);

		v = (-k1 - w) * ik2;
		if(v < 0.0f || v>1.0f)
			v = (-k1 + w) * ik2;

		u = (kp.x - ka.x * v) / (kb.x + kg.x * v);
	}

	if(u < 0.0f || u>1.0f || v < 0.0f || v>1.0f)
		return 0;

	if(outT)
		*outT = t;

	const float hitNormal = rayDir.Dot(nor);
	return hitNormal >= 0 ? -1 : 1;
}

bool RayToElipsis(const Vec3& from, const Vec3& dir, const Vec3& pos, float sizeX, float sizeZ, float& t)
{
	const Plane plane(pos, Vec3::Up);
	if(!RayToPlane(from, dir, plane, &t))
		return false;

	const Vec3 hitpoint = from + dir * t;
	const float dx = hitpoint.x - pos.x;
	const float dz = hitpoint.z - pos.z;
	const float p = (dx * dx) / (sizeX * sizeX) + (dz * dz) / (sizeZ * sizeZ);
	return p <= 1.f;
}

bool RayToSphere(const Vec3& rayPos, const Vec3& rayDir, const Vec3& center, float radius, float& dist)
{
	Vec3 rayPosMinusCenter = rayPos - center;
	float a = rayDir.Dot(rayDir);
	float b = 2.f * rayDir.Dot(rayPosMinusCenter);
	float c = rayPosMinusCenter.Dot(rayPosMinusCenter) - (radius * radius);
	float delta = b * b - 4.f * a * c;

	if(delta < 0.f)
		return false;

	float a2 = 2.f * a;
	float minusB = -b;
	float sqrtDelta = sqrtf(delta);

	dist = (minusB - sqrtDelta) / a2;
	if(dist >= 0.f)
		return true;
	dist = (minusB + sqrtDelta) / a2;
	if(dist >= 0.f)
	{
		dist = 0.f;
		return true;
	}
	return false;
}

// Doesn't do backface culling
bool RayToTriangle(const Vec3& rayPos, const Vec3& rayDir, const Vec3& v1, const Vec3& v2, const Vec3& v3, float& dist)
{
	Vec3 tvec, pvec, qvec;

	// find vectors for two edges sharing vert0
	Vec3 edge1 = v2 - v1;
	Vec3 edge2 = v3 - v1;

	// begin calculating determinant - also used to calculate U parameter
	pvec = rayDir.Cross(edge2);

	// if determinant is near zero, ray lies in plane of triangle
	float det = edge1.Dot(pvec);
	if(AlmostZero(det))
		return false;
	float invDet = 1.0f / det;

	// calculate distance from vert0 to ray origin
	tvec = rayPos - v1;

	// calculate U parameter and test bounds
	float u = tvec.Dot(pvec) * invDet;
	if(u < 0.0f || u > 1.0f)
		return false;

	// prepare to test V parameter
	qvec = tvec.Cross(edge1);

	// calculate V parameter and test bounds
	float v = rayDir.Dot(qvec) * invDet;
	if(v < 0.0f || u + v > 1.0f)
		return false;

	// calculate t, ray intersects triangle
	dist = edge2.Dot(qvec) * invDet;
	return true;
}

bool LineToLine(const Vec2& start1, const Vec2& end1, const Vec2& start2, const Vec2& end2, float* t)
{
	float uaT = (end2.x - start2.x) * (start1.y - start2.y) - (end2.y - start2.y) * (start1.x - start2.x);
	float ubT = (end1.x - start1.x) * (start1.y - start2.y) - (end1.y - start1.y) * (start1.x - start2.x);
	float uB = (end2.y - start2.y) * (end1.x - start1.x) - (end2.x - start2.x) * (end1.y - start1.y);

	if(uB != 0)
	{
		float ua = uaT / uB;
		float ub = ubT / uB;
		if(0 <= ua && ua <= 1 && 0 <= ub && ub <= 1)
		{
			if(t)
				*t = ua;
			return true;
		}
		else
			return false;
	}
	else
	{
		if(uaT == 0 || ubT == 0)
		{
			if(t)
				*t = 0;
			return true;
		}
		else
			return false;
	}
}

bool LineToRectangle(const Vec2& start, const Vec2& end, const Vec2& rectPos, const Vec2& rectPos2, float* tResult)
{
	assert(rectPos.x <= rectPos2.x && rectPos.y <= rectPos2.y);

	const Vec2 topRight(rectPos2.x, rectPos.y),
		bottomLeft(rectPos.x, rectPos2.y);

	if(tResult)
	{
		float tt, t = 1.001f;

		if(LineToLine(start, end, rectPos, topRight, &tt) && tt < t) t = tt;
		if(LineToLine(start, end, topRight, rectPos2, &tt) && tt < t) t = tt;
		if(LineToLine(start, end, rectPos2, bottomLeft, &tt) && tt < t) t = tt;
		if(LineToLine(start, end, bottomLeft, rectPos, &tt) && tt < t) t = tt;

		*tResult = t;

		return (t <= 1.f);
	}
	else
	{
		if(LineToLine(rectPos, topRight, start, end))    return true;
		if(LineToLine(topRight, rectPos2, start, end))   return true;
		if(LineToLine(rectPos2, bottomLeft, start, end)) return true;
		if(LineToLine(bottomLeft, rectPos, start, end))  return true;

		return false;
	}
}

void CreateAABBOX(Box& out, const Matrix& matrix)
{
	Vec3 v1 = Vec3::Transform(Vec3(-2, -2, -2), matrix),
		v2 = Vec3::Transform(Vec3(2, 2, 2), matrix);
	out.v1 = v1;
	out.v2 = v2;
}

bool BoxToBox(const Box& box1, const Box& box2)
{
	return (box1.v1.x <= box2.v2.x) && (box1.v2.x >= box2.v1.x)
		&& (box1.v1.y <= box2.v2.y) && (box1.v2.y >= box2.v1.y)
		&& (box1.v1.z <= box2.v2.z) && (box1.v2.z >= box2.v1.z);
}

bool RectangleToRectangle(float x1, float y1, float x2, float y2, float a1, float b1, float a2, float b2)
{
	return (x1 <= a2) && (x2 >= a1) && (y1 <= b2) && (y2 >= b1);
}

void ClosestPointInBox(Vec3 *Out, const Box &Box, const Vec3 &p)
{
	Out->x = Clamp(p.x, Box.v1.x, Box.v2.x);
	Out->y = Clamp(p.y, Box.v1.y, Box.v2.y);
	Out->z = Clamp(p.z, Box.v1.z, Box.v2.z);
}

bool SphereToBox(const Vec3 &SphereCenter, float SphereRadius, const Box &Box)
{
	Vec3 PointInBox;
	ClosestPointInBox(&PointInBox, Box, SphereCenter);
	return Vec3::DistanceSquared(SphereCenter, PointInBox) < SphereRadius * SphereRadius;
}

bool CircleToRotatedRectangle(float cx, float cy, float radius, float rx, float ry, float w, float h, float rot)
{
	// transform circle so rectangle don't need rotation
	float sina = sin(rot),
		cosa = cos(rot),
		difx = cx - rx,
		dify = cy - ry,
		x = cosa * difx - sina * dify + rx,
		y = sina * difx + cosa * dify + ry;
	return CircleToRectangle(x, y, radius, rx, ry, w, h);
}

inline void RotateVector2DClockwise(Vec2& v, float ang)
{
	float t,
		cosa = cos(ang),
		sina = sin(ang);
	t = v.x;
	v.x = t * cosa + v.y * sina;
	v.y = -t * sina + v.y * cosa;
}

// Rotated Rectangles Collision Detection, Oren Becker, 2001
// http://ragestorm.net/samples/CDRR.C
bool RotatedRectanglesCollision(const RotRect& r1, const RotRect& r2)
{
	Vec2 A, B,   // vertices of the rotated rr2
		C,      // center of rr2
		BL, TR; // vertices of rr2 (bottom-left, top-right)

	float ang = r1.rot - r2.rot, // orientation of rotated rr1
		cosa = cos(ang),           // precalculated trigonometic -
		sina = sin(ang);           // - values for repeated use

	float t, x, a;      // temporary variables for various uses
	float dx;           // deltaX for linear equations
	float ext1, ext2;   // min/max vertical values

	// move r2 to make r1 cannonic
	C = r2.center - r1.center;

	// rotate r2 clockwise by r2.ang to make r2 axis-aligned
	RotateVector2DClockwise(C, r2.rot);

	// calculate vertices of (moved and axis-aligned := 'ma') r2
	BL = TR = C;
	BL -= r2.size;
	TR += r2.size;

	// calculate vertices of (rotated := 'r') r1
	A.x = -r1.size.y * sina; B.x = A.x; t = r1.size.x * cosa; A.x += t; B.x -= t;
	A.y = r1.size.y * cosa; B.y = A.y; t = r1.size.x * sina; A.y += t; B.y -= t;

	t = sina * cosa;

	// verify that A is vertical min/max, B is horizontal min/max
	if(t < 0)
	{
		t = A.x; A.x = B.x; B.x = t;
		t = A.y; A.y = B.y; B.y = t;
	}

	// verify that B is horizontal minimum (leftest-vertex)
	if(sina < 0) { B.x = -B.x; B.y = -B.y; }

	// if r2(ma) isn't in the horizontal range of
	// colliding with r1(r), collision is impossible
	if(B.x > TR.x || B.x > -BL.x) return false;

	// if r1(r) is axis-aligned, vertical min/max are easy to get
	if(t == 0) { ext1 = A.y; ext2 = -ext1; }
	// else, find vertical min/max in the range [BL.x, TR.x]
	else
	{
		x = BL.x - A.x; a = TR.x - A.x;
		ext1 = A.y;
		// if the first vertical min/max isn't in (BL.x, TR.x), then
		// find the vertical min/max on BL.x or on TR.x
		if(a * x > 0)
		{
			dx = A.x;
			if(x < 0) { dx -= B.x; ext1 -= B.y; x = a; }
			else { dx += B.x; ext1 += B.y; }
			ext1 *= x; ext1 /= dx; ext1 += A.y;
		}

		x = BL.x + A.x; a = TR.x + A.x;
		ext2 = -A.y;
		// if the second vertical min/max isn't in (BL.x, TR.x), then
		// find the local vertical min/max on BL.x or on TR.x
		if(a * x > 0)
		{
			dx = -A.x;
			if(x < 0) { dx -= B.x; ext2 -= B.y; x = a; }
			else { dx += B.x; ext2 += B.y; }
			ext2 *= x; ext2 /= dx; ext2 -= A.y;
		}
	}

	// check whether r2(ma) is in the vertical range of colliding with r1(r)
	// (for the horizontal range of r2)
	return !((ext1 < BL.y && ext2 < BL.y)
		|| (ext1 > TR.y && ext2 > TR.y));
}

// kolizja promienia (A->B) z cylindrem (P->Q, promie� R)
// z Real Time Collision Detection str 197
//----------------------------------------------
// Intersect segment S(t)=sa+t(sb-sa), 0<=t<=1 against cylinder specifiedby p, q and r
int RayToCylinder(const Vec3& sa, const Vec3& sb, const Vec3& p, const Vec3& q, float r, float& t)
{
	Vec3 d = q - p, m = sa - p, n = sb - sa;
	float md = m.Dot(d);
	float nd = n.Dot(d);
	float dd = d.Dot(d);
	// Test if segment fully outside either endcap of cylinder
	if(md < 0.0f && md + nd < 0.0f)
		return 0; // Segment outside 'p' side of cylinder
	if(md > dd && md + nd > dd)
		return 0; // Segment outside 'q' side of cylinder
	float nn = n.Dot(n);
	float mn = m.Dot(n);
	float a = dd * nn - nd * nd;
	float k = m.Dot(m) - r * r;
	float c = dd * k - md * md;
	if(IsZero(a))
	{
		// Segment runs parallel to cylinder axis
		if(c > 0.0f) return 0; // 'a' and thus the segment lie outside cylinder
		// Now known that segment intersects cylinder; figure out how it intersects
		if(md < 0.0f) t = -mn / nn; // Intersect segment against 'p' endcap
		else if(md > dd) t = (nd - mn) / nn; // Intersect segment against 'q' endcap
		else t = 0.0f; // 'a' lies inside cylinder
		return 1;
	}
	float b = dd * mn - nd * md;
	float discr = b * b - a * c;
	if(discr < 0.0f) return 0; // No real roots; no intersection
	t = (-b - sqrt(discr)) / a;
	if(t < 0.0f || t > 1.0f) return 0; // Intersection lies outside segment
	if(md + t * nd < 0.0f)
	{
		// Intersection outside cylinder on 'p' side
		if(nd <= 0.0f) return 0; // Segment pointing away from endcap
		t = -md / nd;
		// Keep intersection if Dot(S(t) - p, S(t) - p) <= r /\ 2
		return k + 2 * t * (mn + t * nn) <= 0.0f;
	}
	else if(md + t * nd > dd)
	{
		// Intersection outside cylinder on 'q' side
		if(nd >= 0.0f) return 0; // Segment pointing away from endcap
		t = (dd - md) / nd;
		// Keep intersection if Dot(S(t) - q, S(t) - q) <= r /\ 2
		return k + dd - 2 * md + t * (2 * (mn - nd) + t * nn) <= 0.0f;
	}
	// Segment intersects cylinder between the endcaps; t is correct
	return 1;
}

struct MATRIX33
{
	Vec3 v[3];

	Vec3& operator [] (int n)
	{
		return v[n];
	}
};

bool OOBToOOB(const Oob& a, const Oob& b)
{
	constexpr float EPSILON = std::numeric_limits<float>::epsilon();

	float ra, rb;
	MATRIX33 R, AbsR;
	// Compute rotation matrix expressing b in a�s coordinate frame
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			R[i][j] = a.u[i].Dot(b.u[j]);
	// Compute translation vector t
	Vec3 t = b.c - a.c;
	// Bring translation into a�s coordinate frame
	t = Vec3(t.Dot(a.u[0]), t.Dot(a.u[2]), t.Dot(a.u[2]));
	// Compute common subexpressions. Add in an epsilon term to
	// counteract arithmetic errors when two edges are parallel and
	// their cross product is (near) null (see text for details)
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			AbsR[i][j] = abs(R[i][j]) + EPSILON;
	// Test axes L = A0, L = A1, L = A2
	for(int i = 0; i < 3; i++) {
		ra = a.e[i];
		rb = b.e[0] * AbsR[i][0] + b.e[1] * AbsR[i][1] + b.e[2] * AbsR[i][2];
		if(abs(t[i]) > ra + rb) return false;
	}
	// Test axes L = B0, L = B1, L = B2
	for(int i = 0; i < 3; i++) {
		ra = a.e[0] * AbsR[0][i] + a.e[1] * AbsR[1][i] + a.e[2] * AbsR[2][i];
		rb = b.e[i];
		if(abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb) return false;
	}
	// Test axis L = A0 x B0
	ra = a.e[1] * AbsR[2][0] + a.e[2] * AbsR[1][0];
	rb = b.e[1] * AbsR[0][2] + b.e[2] * AbsR[0][1];
	if(abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) return false;
	// Test axis L = A0 x B1
	ra = a.e[1] * AbsR[2][1] + a.e[2] * AbsR[1][1];
	rb = b.e[0] * AbsR[0][2] + b.e[2] * AbsR[0][0];
	if(abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) return false;
	// Test axis L = A0 x B2
	ra = a.e[1] * AbsR[2][2] + a.e[2] * AbsR[1][2];
	rb = b.e[0] * AbsR[0][1] + b.e[1] * AbsR[0][0];
	if(abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) return false;
	// Test axis L = A1 x B0
	ra = a.e[0] * AbsR[2][0] + a.e[2] * AbsR[0][0];
	rb = b.e[1] * AbsR[1][2] + b.e[2] * AbsR[1][1];
	if(abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) return false;
	// Test axis L = A1 x B1
	ra = a.e[0] * AbsR[2][1] + a.e[2] * AbsR[0][1];
	rb = b.e[0] * AbsR[1][2] + b.e[2] * AbsR[1][0];
	if(abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) return false;
	// Test axis L = A1 x B2
	ra = a.e[0] * AbsR[2][2] + a.e[2] * AbsR[0][2];
	rb = b.e[0] * AbsR[1][1] + b.e[1] * AbsR[1][0];
	if(abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) return false;
	// Test axis L = A2 x B0
	ra = a.e[0] * AbsR[1][0] + a.e[1] * AbsR[0][0];
	rb = b.e[1] * AbsR[2][2] + b.e[2] * AbsR[2][1];
	if(abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) return false;
	// Test axis L = A2 x B1
	ra = a.e[0] * AbsR[1][1] + a.e[1] * AbsR[0][1];
	rb = b.e[0] * AbsR[2][2] + b.e[2] * AbsR[2][0];
	if(abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) return false;
	// Test axis L = A2 x B2
	ra = a.e[0] * AbsR[1][2] + a.e[1] * AbsR[0][2];
	rb = b.e[0] * AbsR[2][1] + b.e[1] * AbsR[2][0];
	if(abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) return false;
	// Since no separating axis is found, the OBBs must be intersecting
	return true;
}

float DistanceRectangleToPoint(const Vec2& pos, const Vec2& size, const Vec2& pt)
{
	float dx = max(abs(pt.x - pos.x) - size.x / 2, 0.f);
	float dy = max(abs(pt.y - pos.y) - size.y / 2, 0.f);
	return sqrt(dx * dx + dy * dy);
}

float PointLineDistance(float x0, float y0, float x1, float y1, float x2, float y2)
{
	float x = x2 - x1;
	float y = y2 - y1;
	return abs(y * x0 - x * y0 + x2 * y1 - y2 * x1) / sqrt(y * y + x * x);
}

float GetClosestPointOnLineSegment(const Vec2& A, const Vec2& B, const Vec2& P, Vec2& result)
{
	Vec2 AP = P - A;       //Vector from A to P
	Vec2 AB = B - A;       //Vector from A to B

	float magnitudeAB = AB.LengthSquared(); //Magnitude of AB vector (it's length squared)
	float ABAPproduct = AP.Dot(AB); //The DOT product of a_to_p and a_to_b
	float distance = ABAPproduct / magnitudeAB; //The normalized "distance" from a to your closest point

	if(distance < 0)     //Check if P projection is over vectorAB
		result = A;
	else if(distance > 1)
		result = B;
	else
		result = A + AB * distance;

	return PointLineDistance(P.x, P.y, A.x, A.y, B.x, B.y);
}

const Vec2 POISSON_DISC_2D[] = {
	Vec2(-0.6271834f, -0.3647562f),
	Vec2(-0.6959124f, -0.1932297f),
	Vec2(-0.425675f, -0.4331925f),
	Vec2(-0.8259574f, -0.3775373f),
	Vec2(-0.4134415f, -0.2794108f),
	Vec2(-0.6711653f, -0.5842927f),
	Vec2(-0.505241f, -0.5710775f),
	Vec2(-0.5399489f, -0.1941965f),
	Vec2(-0.2056243f, -0.3328375f),
	Vec2(-0.2721521f, -0.4913186f),
	Vec2(0.009952361f, -0.4938473f),
	Vec2(-0.3341284f, -0.7402002f),
	Vec2(-0.009171869f, -0.1417411f),
	Vec2(-0.05370279f, -0.3561031f),
	Vec2(-0.2042215f, -0.1395438f),
	Vec2(0.1491909f, -0.7528881f),
	Vec2(-0.09437386f, -0.6736782f),
	Vec2(0.2218135f, -0.5837499f),
	Vec2(0.1357503f, -0.2823138f),
	Vec2(0.1759486f, -0.4372835f),
	Vec2(-0.8812768f, -0.1270963f),
	Vec2(-0.5861077f, -0.7143953f),
	Vec2(-0.4840448f, -0.8610057f),
	Vec2(-0.1953385f, -0.9313949f),
	Vec2(-0.3544169f, -0.1299241f),
	Vec2(0.4259588f, -0.3359875f),
	Vec2(0.1780135f, -0.006630601f),
	Vec2(0.3781602f, -0.174012f),
	Vec2(-0.6535406f, 0.07830032f),
	Vec2(-0.4176719f, 0.006290245f),
	Vec2(-0.2157413f, 0.1043319f),
	Vec2(-0.3825159f, 0.1611559f),
	Vec2(-0.04609891f, 0.1563928f),
	Vec2(-0.2525779f, 0.3147326f),
	Vec2(0.6283897f, -0.2800752f),
	Vec2(0.5242329f, -0.4569906f),
	Vec2(0.5337259f, -0.1482658f),
	Vec2(0.4243455f, -0.6266792f),
	Vec2(-0.8479414f, 0.08037262f),
	Vec2(-0.5815527f, 0.3148638f),
	Vec2(-0.790419f, 0.2343442f),
	Vec2(-0.4226354f, 0.3095743f),
	Vec2(-0.09465869f, 0.3677911f),
	Vec2(0.3935578f, 0.04151043f),
	Vec2(0.2390065f, 0.1743644f),
	Vec2(0.02775179f, 0.01711585f),
	Vec2(-0.3588479f, 0.4862351f),
	Vec2(-0.7332007f, 0.3809305f),
	Vec2(-0.5283061f, 0.5106883f),
	Vec2(0.7347565f, -0.04643056f),
	Vec2(0.5254471f, 0.1277963f),
	Vec2(-0.1984853f, 0.6903372f),
	Vec2(-0.1512452f, 0.5094652f),
	Vec2(-0.5878937f, 0.6584677f),
	Vec2(-0.4450369f, 0.7685395f),
	Vec2(0.691914f, -0.552465f),
	Vec2(0.293443f, -0.8303219f),
	Vec2(0.5147449f, -0.8018763f),
	Vec2(0.3373911f, -0.4752345f),
	Vec2(-0.7731022f, 0.6132235f),
	Vec2(-0.9054359f, 0.3877104f),
	Vec2(0.1200563f, -0.9095488f),
	Vec2(-0.05998399f, -0.8304204f),
	Vec2(0.1212275f, 0.4447584f),
	Vec2(-0.04844639f, 0.8149281f),
	Vec2(-0.1576151f, 0.9731216f),
	Vec2(-0.2921374f, 0.8280436f),
	Vec2(0.8305115f, -0.3373946f),
	Vec2(0.7025464f, -0.7087887f),
	Vec2(-0.9783711f, 0.1895637f),
	Vec2(-0.9950094f, 0.03602472f),
	Vec2(-0.02693105f, 0.6184058f),
	Vec2(-0.3686568f, 0.6363685f),
	Vec2(0.07644552f, 0.9160427f),
	Vec2(0.2174875f, 0.6892526f),
	Vec2(0.09518065f, 0.2284235f),
	Vec2(0.2566459f, 0.8855528f),
	Vec2(0.2196656f, -0.1571368f),
	Vec2(0.9549446f, -0.2014009f),
	Vec2(0.4562157f, 0.7741205f),
	Vec2(0.3333389f, 0.413012f),
	Vec2(0.5414181f, 0.2789065f),
	Vec2(0.7839744f, 0.2456573f),
	Vec2(0.6805856f, 0.1255756f),
	Vec2(0.3859844f, 0.2440029f),
	Vec2(0.4403853f, 0.600696f),
	Vec2(0.6249176f, 0.6072751f),
	Vec2(0.5145468f, 0.4502719f),
	Vec2(0.749785f, 0.4564187f),
	Vec2(0.9864355f, -0.0429658f),
	Vec2(0.8654963f, 0.04940263f),
	Vec2(0.9577024f, 0.1808657f)
};
const int poissonDiscCount = countof(POISSON_DISC_2D);

Vec2 Vec2::RandomPoissonDiscPoint()
{
	int index = Rand() % poissonDiscCount;
	const Vec2& pos = POISSON_DISC_2D[index];
	return pos;
}

uint Hash(const string& str)
{
	uint hash = 0x811c9dc5;
	uint prime = 0x1000193;

	for(uint i = 0, len = str.size(); i < len; ++i)
	{
		byte value = str[i];
		hash = hash ^ value;
		hash *= prime;
	}

	assert(hash != 0u);
	return hash;
}

uint Hash(cstring str)
{
	uint hash = 0x811c9dc5;
	const uint prime = 0x1000193;

	while(byte value = *str++)
	{
		hash = hash ^ value;
		hash *= prime;
	}

	assert(hash != 0u);
	return hash;
}

uint Hash(const void* ptr, uint size)
{
	uint result;
	MurmurHash3_x86_32(ptr, size, 0, &result);
	return result;
}

cstring Guid::ToString() const
{
	return Format("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		data1, data2, data3, data4[0], data4[1],
		data4[2], data4[3], data4[4], data4[5], data4[6], data4[7]);
}

bool Guid::TryParse(const string& str)
{
	uint _data2, _data3, _data4[8];
	if(sscanf_s(str.c_str(), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		&data1, &_data2, &_data3, &_data4[0], &_data4[1],
		&_data4[2], &_data4[3], &_data4[4], &_data4[5], &_data4[6], &_data4[7]) == 11)
	{
		data2 = _data2;
		data3 = _data3;
		for(int i = 0; i < 8; ++i)
			data4[i] = _data4[i];
		return true;
	}
	return false;
}

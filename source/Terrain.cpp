#include "Pch.h"
#include "Terrain.h"

#include "DirectX.h"
#include "Render.h"
#include "Texture.h"

//-----------------------------------------------------------------------------
void CalculateNormal(VTerrain& v1, VTerrain& v2, VTerrain& v3)
{
	Vec3 v01 = v2.pos - v1.pos;
	Vec3 v02 = v3.pos - v1.pos;
	Vec3 normal = v01.Cross(v02).Normalize();

	v1.normal = normal;
	v2.normal = normal;
	v3.normal = normal;
}

//-----------------------------------------------------------------------------
void CalculateNormal(Vec3& out, const Vec3& v1, const Vec3& v2, const Vec3& v3)
{
	Vec3 v01 = v2 - v1;
	Vec3 v02 = v3 - v1;

	out = v01.Cross(v02).Normalize();
}

//=================================================================================================
Terrain::Terrain() : vb(nullptr), vbStaging(nullptr), ib(nullptr), parts(nullptr), h(nullptr), texSplat(nullptr), tex(), state(0), uvMod(DEFAULT_UV_MOD)
{
}

//=================================================================================================
Terrain::~Terrain()
{
	SafeRelease(vb);
	SafeRelease(vbStaging);
	SafeRelease(ib);
	delete texSplat;
	delete[] parts;
	delete[] h;
}

//=================================================================================================
void Terrain::Init(const Options& o)
{
	assert(state == 0);
	assert(o.tileSize > 0.f && o.nParts > 0 && o.tilesPerPart > 0 && IsPow2(o.texSize));

	pos = o.pos;
	tileSize = o.tileSize;
	nParts = o.nParts;
	nParts2 = nParts * nParts;
	tilesPerPart = o.tilesPerPart;
	nTiles = nParts * tilesPerPart;
	nTiles2 = nTiles * nTiles;
	tilesSize = tileSize * nTiles;
	width = nTiles + 1;
	width2 = width * width;
	nTris = nTiles2 * 2;
	nVerts = nTiles2 * 6;
	partTris = tilesPerPart * tilesPerPart * 2;
	partVerts = tilesPerPart * tilesPerPart * 6;
	texSize = o.texSize;
	box.v1 = pos;
	box.v2 = pos + Vec3(tiles_size, 0, tiles_size);

	h = new float[width2];
	parts = new Part[nParts2];
	Vec3 dif = box.v2 - box.v1;
	dif /= float(nParts);
	dif.y = 0;
	for(uint z = 0; z < nParts; ++z)
	{
		for(uint x = 0; x < nParts; ++x)
		{
			parts[x + z * nParts].box.v1 = box.v1 + Vec3(dif.x * x, 0, dif.z * z);
			parts[x + z * nParts].box.v2 = parts[x + z * nParts].box.v1 + dif;
		}
	}

	texSplat = app::render->CreateDynamicTexture(Int2(texSize));

	state = 1;
}

//=================================================================================================
void Terrain::Build(bool smooth)
{
	assert(state == 1);

	ID3D11Device* device = app::render->GetDevice();

	// create vertex buffer
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VTerrain) * nVerts;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	V(device->CreateBuffer(&bufferDesc, nullptr, &vb));
	SetDebugName(vb, "TerrainVb");

	// create staging vertex buffer
	bufferDesc.Usage = D3D11_USAGE_STAGING;
	bufferDesc.BindFlags = 0;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

	V(device->CreateBuffer(&bufferDesc, nullptr, &vbStaging));
	SetDebugName(vbStaging, "TerrainVbStaging");

	// build mesh
	ID3D11DeviceContext* deviceContext = app::render->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE res;
	V(deviceContext->Map(vbStaging, 0, D3D11_MAP_WRITE, 0, &res));
	VTerrain* v = reinterpret_cast<VTerrain*>(res.pData);

#define TRI(xx,zz,uu,vv) v[n++] = VTerrain((x+xx)*tileSize, h[x+xx+(z+zz)*width], (z+zz)*tileSize, float(uu)/uvMod, float(vv)/uvMod,\
	((float)(x+xx)) / nTiles, ((float)(z+zz)) / nTiles)

	uint n = 0;
	for(uint z = 0; z < nTiles; ++z)
	{
		for(uint x = 0; x < nTiles; ++x)
		{
			int u1 = (x % uvMod);
			int u2 = ((x + 1) % uvMod);
			if(u2 == 0)
				u2 = uvMod;
			int v1 = (z % uvMod);
			int v2 = ((z + 1) % uvMod);
			if(v2 == 0)
				v2 = uvMod;

			TRI(0, 0, u1, v1);
			TRI(0, 1, u1, v2);
			TRI(1, 0, u2, v1);
			CalculateNormal(v[n - 3], v[n - 2], v[n - 1]);

			TRI(0, 1, u1, v2);
			TRI(1, 1, u2, v2);
			TRI(1, 0, u2, v1);
			CalculateNormal(v[n - 3], v[n - 2], v[n - 1]);
		}
	}
#undef TRI

	// fill indices
	Buf buf;
	uint size = sizeof(int) * nTris * 3;
	uint* idx = buf.Get<uint>(size);
	for(uint z = 0; z < nParts; ++z)
	{
		for(uint x = 0; x < nParts; ++x)
		{
			const uint zStart = z * tilesPerPart,
				zEnd = zStart + tilesPerPart,
				xStart = x * tilesPerPart,
				xEnd = xStart + tilesPerPart;

			for(uint zz = zStart; zz < zEnd; ++zz)
			{
				for(uint xx = xStart; xx < xEnd; ++xx)
				{
					for(uint j = 0; j < 6; ++j)
					{
						*idx = (xx + zz * nTiles) * 6 + j;
						++idx;
					}
				}
			}
		}
	}

	// create index buffer
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.ByteWidth = sizeof(int) * nTris * 3;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA subData = {};
	subData.pSysMem = buf.Get();

	V(device->CreateBuffer(&bufferDesc, &subData, &ib));
	SetDebugName(ib, "TerrainIb");

	// smooth mesh
	state = 2;
	if(smooth)
		SmoothNormals(v);
	deviceContext->Unmap(vbStaging, 0);
	deviceContext->CopyResource(vb, vbStaging);
}

//=================================================================================================
void Terrain::Rebuild(bool smooth)
{
	assert(state == 2);

	ID3D11DeviceContext* deviceContext = app::render->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE res;
	V(deviceContext->Map(vbStaging, 0, D3D11_MAP_READ_WRITE, 0, &res));
	VTerrain* v = reinterpret_cast<VTerrain*>(res.pData);

#define TRI(xx,zz) v[n++].pos.y = h[x+xx+(z+zz)*width]

	uint n = 0;
	for(uint z = 0; z < nTiles; ++z)
	{
		for(uint x = 0; x < nTiles; ++x)
		{
			TRI(0, 0);
			TRI(0, 1);
			TRI(1, 0);
			CalculateNormal(v[n - 3], v[n - 2], v[n - 1]);

			TRI(0, 1);
			TRI(1, 1);
			TRI(1, 0);
			CalculateNormal(v[n - 3], v[n - 2], v[n - 1]);
		}
	}
#undef TRI

	if(smooth)
		SmoothNormals(v);

	deviceContext->Unmap(vbStaging, 0);
	deviceContext->CopyResource(vb, vbStaging);
}

//=================================================================================================
void Terrain::RebuildUv()
{
	assert(state == 2);

	ID3D11DeviceContext* deviceContext = app::render->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE res;
	V(deviceContext->Map(vbStaging, 0, D3D11_MAP_READ_WRITE, 0, &res));
	VTerrain* v = reinterpret_cast<VTerrain*>(res.pData);

#define TRI(uu,vv) v[n++].tex = Vec2(float(uu)/uvMod, float(vv)/uvMod)

	uint n = 0;
	for(uint z = 0; z < nTiles; ++z)
	{
		for(uint x = 0; x < nTiles; ++x)
		{
			int u1 = (x % uvMod);
			int u2 = ((x + 1) % uvMod);
			if(u2 == 0)
				u2 = uvMod;
			int v1 = (z % uvMod);
			int v2 = ((z + 1) % uvMod);
			if(v2 == 0)
				v2 = uvMod;

			TRI(u1, v1);
			TRI(u1, v2);
			TRI(u2, v1);

			TRI(u1, v2);
			TRI(u2, v2);
			TRI(u2, v1);
		}
	}
#undef TRI

	deviceContext->Unmap(vbStaging, 0);
	deviceContext->CopyResource(vb, vbStaging);
}

//=================================================================================================
void Terrain::Make(bool smooth)
{
	assert(state != 0);
	if(state == 1)
		Build(smooth);
	else
		Rebuild(smooth);
}

//=================================================================================================
void Terrain::SetHeight(float height)
{
	assert(state > 0);

	for(uint i = 0; i < width2; ++i)
		h[i] = height;

	box.v1.y = height - 0.1f;
	box.v2.y = height + 0.1f;

	for(uint i = 0; i < nParts2; ++i)
	{
		parts[i].box.v1.y = height - 0.1f;
		parts[i].box.v2.y = height + 0.1f;
	}
}

//=================================================================================================
void Terrain::SetBordersHeight(float height)
{
	assert(state > 0);
	for(uint i = 0; i < width; ++i)
	{
		// back
		h[i] = height;
		// front
		h[i + (width - 1) * width] = height;
		// left
		h[i * width] = height;
		// right
		h[width - 1 + i * width] = height;
	}
}

//=================================================================================================
void Terrain::RandomizeHeight(float hmin, float hmax)
{
	assert(state > 0);
	assert(hmin < hmax);

	for(uint i = 0; i < width2; ++i)
		h[i] = Random(hmin, hmax);
}

//=================================================================================================
void Terrain::RoundHeight()
{
	assert(state > 0);

#define H(xx,zz) h[x+(xx)+(z+(zz))*width]

	// aby przyœpieszyæ t¹ funkcje mo¿na by obliczyæ najpierw dla krawêdzi a
	// potem œrodek w osobnych pêtlach, wtedy nie potrzeba by by³o ¿adnych
	// warunków, na razie jest to jednak nie potrzebna bo ta funkcja nie jest
	// u¿ywana realtime tylko w edytorze przy tworzeniu losowego terenu
	for(uint z = 0; z < width; ++z)
	{
		for(uint x = 0; x < width; ++x)
		{
			int count = 1;
			float sum = H(0, 0);

			// wysokoœæ z prawej
			if(x < width - 1)
			{
				sum += H(1, 0);
				++count;
			}
			// wysokoœæ z lewej
			if(x > 0)
			{
				sum += H(-1, 0);
				++count;
			}
			// wysokoœæ z góry
			if(z < width - 1)
			{
				sum += H(0, 1);
				++count;
			}
			// wysokoœæ z do³u
			if(z > 0)
			{
				sum += H(0, -1);
				++count;
			}
			// mo¿na by dodaæ jeszcze elementy na ukos
			H(0, 0) = sum / count;
		}
	}
}


//=================================================================================================
void Terrain::RoundHeightWithoutBorders()
{
	assert(state > 0);

	for(uint z = 1; z < width - 1; ++z)
	{
		for(uint x = 1; x < width - 1; ++x)
		{
			float sum = H(0, 0) + H(1, 0) + H(-1, 0) + H(0, 1) + H(0, -1);
			H(0, 0) = sum / 5;
		}
	}
}

//=================================================================================================
void Terrain::CalculateBox()
{
	assert(state > 0);

	float smax = -Inf(),
		smin = Inf();
	for(uint i = 0; i < nParts * nParts; ++i)
	{
		float pmax = -Inf();
		float pmin = Inf();

		const uint zStart = (i / nParts) * tilesPerPart,
			zEnd = zStart + tilesPerPart,
			xStart = (i % nParts) * tilesPerPart,
			xEnd = xStart + tilesPerPart;

		for(uint z = zStart; z <= zEnd; ++z)
		{
			for(uint x = xStart; x <= xEnd; ++x)
			{
				float hc = h[x + z * width];
				if(hc > pmax)
					pmax = hc;
				if(hc < pmin)
					pmin = hc;
			}
		}

		pmin -= 0.1f;
		pmax += 0.1f;

		parts[i].box.v1.y = pmin;
		parts[i].box.v2.y = pmax;

		if(pmax > smax)
			smax = pmax;
		if(pmin < smin)
			smin = pmin;
	}

	box.v1.y = smin;
	box.v2.y = smax;
}

//=================================================================================================
void Terrain::Randomize()
{
	assert(state > 0);

	RandomizeHeight(0.f, 8.f);
	RoundHeight();
	RoundHeight();
	RoundHeight();
	CalculateBox();
}

//=================================================================================================
void Terrain::SmoothNormals()
{
	assert(state > 0);

	ID3D11DeviceContext* deviceContext = app::render->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE res;
	V(deviceContext->Map(vbStaging, 0, D3D11_MAP_READ_WRITE, 0, &res));
	VTerrain* v = reinterpret_cast<VTerrain*>(res.pData);

	SmoothNormals(v);

	deviceContext->Unmap(vbStaging, 0);
	deviceContext->CopyResource(vb, vbStaging);
}

//=================================================================================================
void Terrain::SmoothNormals(VTerrain* v)
{
	assert(state > 0);
	assert(v);

	Vec3 normal, normal2;
	uint sum;

	// 1,3         4
	//(0,1)		 (1,1)
	// O-----------O
	// |\          |
	// |  \        |
	// |    \      |
	// |      \    |
	// |        \  |
	// |          \|
	// O-----------O
	//(0,0)		 (1,0)
	//  0         2,5

#undef W
#define W(xx,zz,idx) v[(x+(xx)+(z+(zz))*nTiles)*6+(idx)]
#define CalcNormal(xx,zz,i0,i1,i2) CalculateNormal(normal2,W(xx,zz,i0).pos,W(xx,zz,i1).pos,W(xx,zz,i2).pos);\
	normal += normal2

	// wyg³adŸ normalne
	for(uint z = 0; z < nTiles; ++z)
	{
		for(uint x = 0; x < nTiles; ++x)
		{
			bool hasLeft = (x > 0),
				hasRight = (x < nTiles - 1),
				hasBottom = (z > 0),
				hasTop = (z < nTiles - 1);

			//------------------------------------------
			// PUNKT (0,0)
			sum = 1;
			normal = W(0, 0, 0).normal;

			if(hasLeft)
			{
				CalcNormal(-1, 0, 0, 1, 2);
				CalcNormal(-1, 0, 3, 4, 5);
				sum += 2;

				if(hasBottom)
				{
					CalcNormal(-1, -1, 3, 4, 5);
					++sum;
				}
			}
			if(hasBottom)
			{
				CalcNormal(0, -1, 0, 1, 2);
				CalcNormal(0, -1, 3, 4, 5);
				sum += 2;
			}

			normal *= (1.f / sum);
			W(0, 0, 0).normal = normal;

			//------------------------------------------
			// PUNKT (0,1)
			normal = W(0, 0, 1).normal;
			normal += W(0, 0, 3).normal;
			sum = 2;

			if(hasLeft)
			{
				CalcNormal(-1, 0, 3, 4, 5);
				++sum;

				if(hasTop)
				{
					CalcNormal(-1, 1, 0, 1, 2);
					CalcNormal(-1, 1, 3, 4, 5);
					sum += 2;
				}
			}
			if(hasTop)
			{
				CalcNormal(0, 1, 0, 1, 2);
				++sum;
			}

			normal *= (1.f / sum);
			W(0, 0, 1).normal = normal;
			W(0, 0, 3).normal = normal;

			//------------------------------------------
			// PUNKT (1,0)
			normal = W(0, 0, 2).normal;
			normal += W(0, 0, 5).normal;
			sum = 2;

			if(hasRight)
			{
				CalcNormal(1, 0, 0, 1, 2);
				++sum;

				if(hasBottom)
				{
					CalcNormal(1, -1, 0, 1, 2);
					CalcNormal(1, -1, 3, 4, 5);
					sum += 2;
				}
			}
			if(hasBottom)
			{
				CalcNormal(0, -1, 3, 4, 5);
				++sum;
			}

			normal *= (1.f / sum);
			W(0, 0, 2).normal = normal;
			W(0, 0, 5).normal = normal;

			//------------------------------------------
			// PUNKT (1,1)
			normal = W(0, 0, 4).normal;
			sum = 1;

			if(hasRight)
			{
				CalcNormal(1, 0, 0, 1, 2);
				CalcNormal(1, 0, 3, 4, 5);
				sum += 2;

				if(hasTop)
				{
					CalcNormal(1, 1, 0, 1, 2);
					++sum;
				}
			}
			if(hasTop)
			{
				CalcNormal(0, 1, 0, 1, 2);
				CalcNormal(0, 1, 3, 4, 5);
				sum += 2;
			}

			normal *= (1.f / sum);
			W(0, 0, 4).normal = normal;
		}
	}
}

//=================================================================================================
float Terrain::GetH(float x, float z) const
{
	assert(state > 0);
	assert(IsInside(x, z));

	// oblicz które to kafle
	uint tx, tz;
	tx = (uint)floor((x - pos.x) / tile_size);
	tz = (uint)floor((z - pos.z) / tile_size);

	// sprawdŸ czy nie jest to poza terenem
	// teren na samej krawêdzi wykrywa jako b³¹d
	if(tx == nTiles)
		--tx;
	if(tz == nTiles)
		--tz;

	// oblicz offset od kafla do punktu
	float offsetx, offsetz;
	offsetx = ((x - pos.x) - tile_size * tx) / tile_size;
	offsetz = ((z - pos.z) - tile_size * tz) / tile_size;

	// pobierz wysokoœci na krawêdziach
	float hTopLeft = h[tx + (tz + 1) * width];
	float hTopRight = h[tx + 1 + (tz + 1) * width];
	float hBottomLeft = h[tx + tz * width];
	float hBottomRight = h[tx + 1 + tz * width];

	// sprawdŸ który to trójk¹t (prawy górny czy lewy dolny)
	if((offsetx * offsetx + offsetz * offsetz) < ((1 - offsetx) * (1 - offsetx) + (1 - offsetz) * (1 - offsetz)))
	{
		// lewy dolny trójk¹t
		float dX = hBottomRight - hBottomLeft;
		float dZ = hTopLeft - hBottomLeft;
		return dX * offsetx + dZ * offsetz + hBottomLeft;
	}
	else
	{
		// prawy górny trójk¹t
		float dX = hTopLeft - hTopRight;
		float dZ = hBottomRight - hTopRight;
		return dX * (1 - offsetx) + dZ * (1 - offsetz) + hTopRight;
	}
}

//=================================================================================================
void Terrain::GetAngle(float x, float z, Vec3& angle) const
{
	assert(IsInside(x, z));

	// oblicz które to kafle
	uint tx, tz;
	tx = (uint)floor((x - pos.x) / tile_size);
	tz = (uint)floor((z - pos.z) / tile_size);

	// sprawdŸ czy nie jest to poza terenem
	// teren na samej krawêdzi wykrywa jako b³¹d
	if(tx == nTiles)
		--tx;
	if(tz == nTiles)
		--tz;

	// oblicz offset od kafla do punktu
	float offsetx, offsetz;
	offsetx = ((x - pos.x) - tile_size * tx) / tile_size;
	offsetz = ((z - pos.z) - tile_size * tz) / tile_size;

	// pobierz wysokoœci na krawêdziach
	float hTopLeft = h[tx + (tz + 1) * width];
	float hTopRight = h[tx + 1 + (tz + 1) * width];
	float hBottomLeft = h[tx + tz * width];
	float hBottomRight = h[tx + 1 + tz * width];
	Vec3 v1, v2, v3;

	// sprawdŸ który to trójk¹t (prawy górny czy lewy dolny)
	if((offsetx * offsetx + offsetz * offsetz) < ((1 - offsetx) * (1 - offsetx) + (1 - offsetz) * (1 - offsetz)))
	{
		// lewy dolny trójk¹t
		v1 = Vec3(tileSize * tx, hBottomLeft, tileSize * tz);
		v2 = Vec3(tileSize * tx, hTopLeft, tileSize * (tz + 1));
		v3 = Vec3(tileSize * (tx + 1), hBottomRight, tileSize * (tz + 1));
	}
	else
	{
		// prawy górny trójk¹t
		v1 = Vec3(tileSize * tx, hTopLeft, tileSize * (tz + 1));
		v2 = Vec3(tileSize * (tx + 1), hTopRight, tileSize * (tz + 1));
		v3 = Vec3(tileSize * (tx + 1), hBottomRight, tileSize * tz);
	}

	// oblicz wektor normalny dla tych punktów
	Vec3 v01 = v2 - v1;
	Vec3 v02 = v3 - v1;
	angle = v01.Cross(v02).Normalize();
}

//=================================================================================================
void Terrain::FillGeometry(vector<Tri>& tris, vector<Vec3>& verts)
{
	uint vcount = (nTiles + 1) * (nTiles + 1);
	verts.reserve(vcount);

	for(uint z = 0; z <= (nTiles + 1); ++z)
	{
		for(uint x = 0; x <= (nTiles + 1); ++x)
		{
			verts.push_back(Vec3(x * tileSize, h[x + z * width], z * tileSize));
		}
	}

	tris.reserve(nTiles * nTiles * 3);

#define XZ(xx,zz) (x+(xx)+(z+(zz))*(nTiles+1))

	for(uint z = 0; z < nTiles; ++z)
	{
		for(uint x = 0; x < nTiles; ++x)
		{
			tris.push_back(Tri(XZ(0, 0), XZ(0, 1), XZ(1, 0)));
			tris.push_back(Tri(XZ(1, 0), XZ(0, 1), XZ(1, 1)));
		}
	}

#undef XZ
}

//=================================================================================================
void Terrain::FillGeometryPart(vector<Tri>& tris, vector<Vec3>& verts, int px, int pz, const Vec3& offset) const
{
	assert(px >= 0 && pz >= 0 && uint(px) < nParts && uint(pz) < nParts);

	verts.reserve((tilesPerPart + 1) * (tilesPerPart + 1));

	for(uint z = pz * tilesPerPart; z <= (pz + 1) * tilesPerPart; ++z)
	{
		for(uint x = px * tilesPerPart; x <= (px + 1) * tilesPerPart; ++x)
		{
			verts.push_back(Vec3(x * tileSize + offset.x, h[x + z * width] + offset.y, z * tileSize + offset.z));
		}
	}

	tris.reserve(tilesPerPart * tilesPerPart * 3);

#define XZ(xx,zz) (x+(xx)+(z+(zz))*(tilesPerPart+1))

	for(uint z = 0; z < tilesPerPart; ++z)
	{
		for(uint x = 0; x < tilesPerPart; ++x)
		{
			tris.push_back(Tri(XZ(0, 0), XZ(0, 1), XZ(1, 0)));
			tris.push_back(Tri(XZ(1, 0), XZ(0, 1), XZ(1, 1)));
		}
	}

#undef XZ
}

//=================================================================================================
void Terrain::SetTextures(TexturePtr* textures)
{
	assert(textures);
	for(uint i = 0; i < 5; ++i)
		tex[i] = textures[i];
}

//=================================================================================================
void Terrain::RemoveHeightMap(bool _delete)
{
	assert(h);

	if(_delete)
		delete[] h;

	h = nullptr;
}

//=================================================================================================
void Terrain::SetHeightMap(float* _h)
{
	assert(_h);

	h = _h;
}

//=================================================================================================
bool Terrain::IsInside(float x, float z) const
{
	return x >= pos.x
		&& x <= pos.x + tiles_size
		&& z >= pos.z
		&& z <= pos.z + tiles_size;
}

//=================================================================================================
void Terrain::ListVisibleParts(vector<uint>& outParts, const FrustumPlanes& frustum) const
{
	if(frustum.BoxToFrustum(box))
	{
		for(uint i = 0; i < n_parts2; ++i)
		{
			if(frustum.BoxToFrustum(parts[i].box))
				outParts.push_back(i);
		}
	}
}

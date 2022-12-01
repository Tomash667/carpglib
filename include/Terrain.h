#pragma once

//-----------------------------------------------------------------------------
#include "VertexDeclaration.h"

//-----------------------------------------------------------------------------
struct Terrain
{
	friend class TerrainShader;
public:
	//---------------------------
	struct Options
	{
		float tileSize;
		uint nParts;
		uint tilesPerPart;
		uint texSize;
	};

	//---------------------------
	struct Tri
	{
		int a, b, c;

		Tri() {}
		Tri(int a, int b, int c) : a(a), b(b), c(c) {}
	};

	//---------------------------
	class Part
	{
		friend struct Terrain;
	public:
		const Box& GetBox() const
		{
			return box;
		}

	private:
		Box box;
	};

	static const int DEFAULT_UV_MOD = 2;
	int uv_mod;

	//---------------------------
	Terrain();
	~Terrain();

	//---------------------------
	void Init(const Options& options);
	void Build(bool smooth = true);
	void Rebuild(bool smooth = true);
	void RebuildUv();
	void Make(bool smooth = true);
	void SetHeight(float height);
	void ClearHeight() { SetHeight(0.f); }
	void RandomizeHeight(float hmin, float hmax);
	void RoundHeight();
	void Randomize();
	void CalculateBox();
	void SmoothNormals();
	void SmoothNormals(VTerrain* v);
	void FillGeometry(vector<Tri>& tris, vector<Vec3>& verts);
	void FillGeometryPart(vector<Tri>& tris, vector<Vec3>& verts, int px, int pz, const Vec3& offset = Vec3(0, 0, 0)) const;

	//---------------------------
	Part* GetPart(uint idx)
	{
		assert(idx < nParts2);
		return &parts[idx];
	}
	float GetH(float x, float z) const;
	float GetH(const Vec3& v) const { return GetH(v.x, v.z); }
	float GetH(const Vec2& v) const { return GetH(v.x, v.y); }
	void SetY(Vec3& v) const { v.y = GetH(v.x, v.z); }
	void GetAngle(float x, float z, Vec3& angle) const;
	uint GetPartsCount() const { return nParts2; }
	DynamicTexture& GetSplatTexture() { return *texSplat; }
	TexturePtr* GetTextures() { return tex; }
	const Box& GetBox() const { return box; }
	const Vec3& GetPos() const { return pos; }
	float* GetHeightMap() { return h; }
	uint GetTerrainWidth() const { return width; }
	uint GetTilesCount() const { return nTiles; }
	uint GetSplatSize() const { return texSize; }
	float GetPartSize() const { return tilesSize / nParts; }
	float GetTileSize() const { return tileSize; }

	//---------------------------
	void SetTextures(TexturePtr* textures);
	void RemoveHeightMap(bool _delete = false);
	void SetHeightMap(float* h);
	bool IsInside(float x, float z) const { return x >= 0.f && z >= 0.f && x < tilesSize && z < tilesSize; }
	bool IsInside(const Vec3& v) const { return IsInside(v.x, v.z); }
	void ListVisibleParts(vector<uint>& parts, const FrustumPlanes& frustum) const;

private:
	Part* parts;
	float* h;
	float tileSize; // rozmiar jednego ma³ego kwadraciku terenu
	float tilesSize;
	uint nTiles, nTiles2; // liczba kwadracików na boku, wszystkich
	uint nParts, nParts2; // liczba sektorów na boku, wszystkich
	uint tilesPerPart;
	uint width, width2; // nTiles+1
	uint nTris, nVerts, partTris, partVerts, texSize;
	Box box;
	ID3D11Buffer* vb;
	ID3D11Buffer* vbStaging;
	ID3D11Buffer* ib;
	DynamicTexture* texSplat;
	TexturePtr tex[5];
	Vec3 pos;
	int state;
};

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
		Options() : pos(Vec3::Zero) {}

		Vec3 pos;
		float tile_size;
		uint n_parts;
		uint tiles_per_part;
		uint tex_size;
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
	void SetBordersHeight(float height);
	void ClearHeight() { SetHeight(0.f); }
	void RandomizeHeight(float hmin, float hmax);
	void RoundHeight();
	void RoundHeightWithoutBorders();
	void Randomize();
	void CalculateBox();
	void SmoothNormals();
	void SmoothNormals(VTerrain* v);
	void FillGeometry(vector<Tri>& tris, vector<Vec3>& verts);
	void FillGeometryPart(vector<Tri>& tris, vector<Vec3>& verts, int px, int pz, const Vec3& offset = Vec3(0, 0, 0)) const;

	//---------------------------
	Part* GetPart(uint idx)
	{
		assert(idx < n_parts2);
		return &parts[idx];
	}
	float GetH(float x, float z) const;
	float GetH(const Vec3& v) const { return GetH(v.x, v.z); }
	float GetH(const Vec2& v) const { return GetH(v.x, v.y); }
	void SetY(Vec3& v) const { v.y = GetH(v.x, v.z); }
	void GetAngle(float x, float z, Vec3& angle) const;
	uint GetPartsCount() const { return n_parts2; }
	DynamicTexture& GetSplatTexture() { return *texSplat; }
	TexturePtr* GetTextures() { return tex; }
	const Box& GetBox() const { return box; }
	const Vec3& GetPos() const { return pos; }
	float* GetHeightMap() { return h; }
	uint GetTerrainWidth() const { return width; }
	uint GetTilesCount() const { return n_tiles; }
	uint GetSplatSize() const { return tex_size; }
	float GetPartSize() const { return tiles_size / n_parts; }
	float GetTileSize() const { return tile_size; }

	//---------------------------
	void SetTextures(TexturePtr* textures);
	void RemoveHeightMap(bool _delete = false);
	void SetHeightMap(float* h);
	bool IsInside(float x, float z) const;
	bool IsInside(const Vec3& v) const { return IsInside(v.x, v.z); }
	void ListVisibleParts(vector<uint>& parts, const FrustumPlanes& frustum) const;

private:
	Part* parts;
	float* h;
	float tile_size; // rozmiar jednego ma³ego kwadraciku terenu
	float tiles_size;
	uint n_tiles, n_tiles2; // liczba kwadracików na boku, wszystkich
	uint n_parts, n_parts2; // liczba sektorów na boku, wszystkich
	uint tiles_per_part;
	uint width, width2; // n_tiles+1
	uint n_tris, n_verts, part_tris, part_verts, tex_size;
	Box box;
	ID3D11Buffer* vb;
	ID3D11Buffer* vbStaging;
	ID3D11Buffer* ib;
	DynamicTexture* texSplat;
	TexturePtr tex[5];
	Vec3 pos;
	int state;
};

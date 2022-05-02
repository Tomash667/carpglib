#pragma once

//-----------------------------------------------------------------------------
#include "VertexData.h"
#include "VertexDeclaration.h"
#include "Texture.h"

/*---------------------------
NAST�PNA WERSJA:
- kolor zamiast tekstury
- FVF i vertex size w nag��wku
- wczytywanie ze strumienia
- brak model_to_bone, odrazu policzone
- lepsza organizacja pliku, wczytywanie wszystkiego za jednym zamachem i ustawianie wska�nik�w
- brak zerowej ko�ci bo i po co
- materia�y
- mo�liwe 32 bit indices jako flaga
- lepsze Predraw
*/
struct Mesh : public Resource
{
	static const ResourceType Type = ResourceType::Mesh;
	static constexpr int MAX_BONES = 64;

	enum MESH_FLAGS
	{
		F_TANGENTS = 1 << 0,
		F_ANIMATED = 1 << 1,
		F_STATIC = 1 << 2,
		F_PHYSICS = 1 << 3,
		F_SPLIT = 1 << 4,
		F_NORMAL_MAP = 1 << 5,
		F_SPECULAR_MAP = 1 << 6
	};

	struct Header
	{
		char format[4];
		byte version, flags;
		word n_verts, n_tris, n_subs, n_bones, n_anims, n_points, n_groups;
		float radius;
		Box bbox;
		uint points_offset;
		Vec3 cam_pos, cam_target, cam_up;
	};

	struct Submesh
	{
		word first; // pierwszy tr�jk�t do narysowania
		word tris; // ile tr�jk�t�w narysowa�
		word min_ind; // odpowiednik parametru DrawIndexedPrimitive - MinIndex (tylko wyra�ony w tr�jk�tach)
		word n_ind; // odpowiednik parametru DrawIndexedPrimitive - NumVertices (tylko wyra�ony w tr�jk�tach)
		string name;//, normal_name, specular_name;
		TexturePtr tex, tex_normal, tex_specular;
		Vec3 specular_color;
		float specular_intensity;
		int specular_hardness;
		float normal_factor, specular_factor, specular_color_factor;

		static const uint MIN_SIZE = 10;
	};

	struct BoneGroup
	{
		word parent;
		string name;
		vector<byte> bones;

		static const uint MIN_SIZE = 4;
	};

	struct Bone
	{
		word id;
		word parent;
		Matrix mat;
		string name;
		vector<word> childs;

		static const uint MIN_SIZE = 51;
	};

	struct KeyframeBone
	{
		Vec3 pos;
		Quat rot;
		Vec3 scale;

		KeyframeBone() {}
		constexpr KeyframeBone(const Vec3& pos, const Quat& rot, const Vec3& scale) : pos(pos), rot(rot), scale(scale) {}
		void Mix(Matrix& out, const Matrix& mul) const;
		static void Interpolate(KeyframeBone& out, const KeyframeBone& k, const KeyframeBone& k2, float t);
	};

	struct Keyframe
	{
		float time;
		vector<KeyframeBone> bones;
	};

	struct Animation
	{
		string name;
		float length;
		word n_frames;
		vector<Keyframe> frames;

		static const uint MIN_SIZE = 7;

		int GetFrameIndex(float time, bool& hit);
	};

	struct Point
	{
		enum Type : word
		{
			PLAIN_AXES,
			SPHERE,
			BOX,
			ARROWS,
			SINGLE_ARROW,
			CIRCLE,
			CONE
		};

		string name;
		Matrix mat;
		Vec3 rot;
		word bone;
		Type type;
		Vec3 size;

		static const uint MIN_SIZE = 73;

		bool IsSphere() const { return type == SPHERE; }
		bool IsBox() const { return type == BOX; }
	};

	struct Split
	{
		Vec3 pos;
		float radius;
		Box box;
	};

	Mesh();
	~Mesh();

	void SetupBoneMatrices();
	void Load(StreamReader& stream, ID3D11Device* device);
	void LoadMetadata(StreamReader& stream);
	void LoadHeader(StreamReader& stream);
	void SetVertexSizeDecl();
	void LoadPoints(StreamReader& stream);
	void LoadBoneGroups(StreamReader& stream);
	void LoadMatrix33(StreamReader& stream, Matrix& m);
	void LoadVertexData(VertexData* vd, StreamReader& stream);
	Animation* GetAnimation(cstring name);
	int GetAnimationIndex(Animation* anim) const;
	Bone* GetBone(cstring name);
	Point* GetPoint(cstring name);
	TEX GetTexture(uint idx) const
	{
		assert(idx < head.n_subs && subs[idx].tex && subs[idx].tex->tex);
		return subs[idx].tex->tex;
	}
	TEX GetTexture(uint index, const TexOverride* tex_override) const
	{
		if(tex_override && tex_override[index].diffuse)
			return tex_override[index].diffuse->tex;
		else
			return GetTexture(index);
	}

	bool IsAnimated() const { return IsSet(head.flags, F_ANIMATED); }
	bool IsStatic() const { return IsSet(head.flags, F_STATIC); }

	void GetKeyframeData(KeyframeBone& keyframe, Animation* ani, uint bone, float time);
	// je�li szuka hit to zwr�ci te� dla hit1, hit____ itp (u�ywane dla box�w broni kt�re si� powtarzaj�)
	Point* FindPoint(cstring name);
	Point* FindNextPoint(cstring name, Point* point);

	Header head;
	ID3D11Buffer* vb;
	ID3D11Buffer* ib;
	VertexDeclarationId vertex_decl;
	uint vertex_size;
	vector<Submesh> subs;
	vector<Bone> bones;
	vector<Animation> anims;
	vector<Matrix> model_to_bone;
	vector<Point> attach_points;
	vector<BoneGroup> groups;
	vector<Split> splits;
};

//-----------------------------------------------------------------------------
struct SimpleMesh
{
	SimpleMesh() : vb(nullptr), ib(nullptr) {}
	~SimpleMesh();
	void Build();
	void Clear();

	ID3D11Buffer* vb;
	ID3D11Buffer* ib;
	vector<Vec3> vertices;
	vector<word> indices;
};

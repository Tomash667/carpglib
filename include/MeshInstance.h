#pragma once

//-----------------------------------------------------------------------------
#include "Mesh.h"

//-----------------------------------------------------------------------------
// flagi u¿ywane przy odtwarzaniu animacji
enum PLAY_FLAGS
{
	// odtwarzaj raz, w przeciwnym razie jest zapêtlone
	// po odtworzeniu animacji na grupie wywo³ywana jest funkcja Deactive(), jeœli jest to grupa
	// podrzêdna to nadrzêdna jest odgrywana
	PLAY_ONCE = 0x01,
	// odtwarzaj od ty³u
	PLAY_BACK = 0x02,
	// wy³¹cza blending dla tej animacji
	PLAY_NO_BLEND = 0x04,
	// ignoruje wywo³anie Play() je¿eli jest ju¿ ta animacja
	PLAY_IGNORE = 0x08,
	PLAY_STOP_AT_END = 0x10,
	// priorytet animacji
	PLAY_PRIO0 = 0,
	PLAY_PRIO1 = 0x20,
	PLAY_PRIO2 = 0x40,
	PLAY_PRIO3 = 0x60,
	// odtwarza animacjê gdy skoñczy siê blending
	PLAY_BLEND_WAIT = 0x100
};

//-----------------------------------------------------------------------------
// obiekt wykorzystuj¹cy Mesh
// zwalnia tekstury override przy niszczeniu
//-----------------------------------------------------------------------------
struct MeshInstance
{
	enum FLAGS
	{
		FLAG_PLAYING = 1 << 0,
		FLAG_ONCE = 1 << 1,
		FLAG_BACK = 1 << 2,
		FLAG_GROUP_ACTIVE = 1 << 3,
		FLAG_BLENDING = 1 << 4,
		FLAG_STOP_AT_END = 1 << 5,
		FLAG_BLEND_WAIT = 1 << 6,
		FLAG_UPDATED = 1 << 7
		// jeœli bêdzie wiêcej flagi potrzeba zmian w Read/Write
	};

	struct Group
	{
		Group() : anim(nullptr), state(0), speed(1.f), prio(0), blendMax(0.33f), frameEnd(false)
		{
		}

		float time, speed, blendTime, blendMax;
		int state, prio, usedGroup;
		union
		{
			Mesh::Animation* anim;
			string* animName; // on preload
		};
		bool frameEnd;

		int GetFrameIndex(bool& hit) const { return anim->GetFrameIndex(time, hit); }
		float GetBlendT() const;
		float GetProgress() const { return time / anim->length; }
		bool IsActive() const { return IsSet(state, FLAG_GROUP_ACTIVE); }
		bool IsBlending() const { return IsSet(state, FLAG_BLENDING); }
		bool IsPlaying() const { return IsSet(state, FLAG_PLAYING); }
		void SetProgress(float progress) { time = progress * anim->length; }
	};
	typedef vector<byte>::const_iterator BoneIter;

	explicit MeshInstance(nullptr_t) : preload(true), mesh(nullptr), needUpdate(true), ptr(nullptr), baseSpeed(1.f), matScale(nullptr) {}
	explicit MeshInstance(Mesh* mesh);
	void Play(Mesh::Animation* anim, int flags, uint group = 0);
	void Play(cstring name, int flags, uint group = 0)
	{
		Mesh::Animation* anim = mesh->GetAnimation(name);
		assert(anim);
		Play(anim, flags, group);
	}
	void Play(uint group = 0) { SetBit(GetGroup(group).state, FLAG_PLAYING); }
	void Stop(uint group = 0) { ClearBit(GetGroup(group).state, FLAG_PLAYING); }
	bool IsPlaying(uint group = 0) const { return GetGroup(group).IsPlaying(); }
	void Deactivate(uint group = 0, bool in_update = false);
	void Update(float dt);
	void SetupBones();
	void DisableAnimations();
	void SetToEnd(cstring anim)
	{
		Mesh::Animation* a = mesh->GetAnimation(anim);
		SetToEnd(a);
	}
	void SetToEnd(Mesh::Animation* anim);
	void SetToEnd();
	void ResetAnimation();
	void Save(FileWriter& f) const;
	void SaveV2(StreamWriter& f) const;
	void Load(FileReader& f, int version);
	void LoadV2(StreamReader& f);
	void LoadSimple(FileReader& f);
	void Write(StreamWriter& f) const;
	bool Read(StreamReader& f);
	bool ApplyPreload(Mesh* mesh);
	void ClearEndResult();
	void Changed() { needUpdate = true; }

	const vector<Matrix>& GetBoneMatrices() const { return matBones; }
	const Matrix& GetBoneMatrix(uint bone) const { return matBones[bone]; }
	Group& GetGroup(uint group)
	{
		assert(group < mesh->head.n_groups);
		return groups[group];
	}
	const Group& GetGroup(uint group) const
	{
		assert(group < mesh->head.n_groups);
		return groups[group];
	}
	int GetHighestPriority(uint& group);
	Mesh* GetMesh() const { return mesh; }
	int GetUsableGroup(uint group);
	float GetProgress(uint group = 0) const { return GetGroup(group).GetProgress(); }

	bool IsActive(uint group = 0) const { return GetGroup(group).IsActive(); }
	bool IsBlending() const;
	bool IsEnded(uint group = 0) const { return GetGroup(group).frameEnd; }

	void SetProgress(float progress, uint group = 0) { GetGroup(group).SetProgress(progress); }

	static void SaveOptional(StreamWriter& f, MeshInstance* meshInst);
	static void LoadOptional(StreamReader& f, MeshInstance*& meshInst);

private:
	void SetupBlending(uint group, bool first = true, bool in_update = false);

	static void(*Predraw)(void*, Matrix*, int);
	Mesh* mesh;
	vector<Matrix> matBones;
	vector<Mesh::KeyframeBone> blendb;
	vector<Group> groups;
	Matrix* matScale;
	void* ptr;
	float baseSpeed;
	bool needUpdate, preload;
};

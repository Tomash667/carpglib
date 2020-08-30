#pragma once

//-----------------------------------------------------------------------------
#include "Entity.h"

//-----------------------------------------------------------------------------
struct Billboard
{
	Texture* tex;
	Vec3 pos;
	float size;
};

//-----------------------------------------------------------------------------
struct ParticleEmitter : public EntityType<ParticleEmitter>
{
	enum PARTICLE_OP
	{
		POP_CONST,
		POP_LINEAR_SHRINK
	};

	struct Particle
	{
		Vec3 pos, speed;
		float life, gravity;
		bool exists;
	};

	TexturePtr tex;
	float emission_interval, life, particle_life, alpha, size;
	int emissions, spawn_min, spawn_max, max_particles, mode;
	Vec3 pos, speed_min, speed_max, pos_min, pos_max;
	PARTICLE_OP op_size, op_alpha;

	// nowe wartoœci, dla kompatybilnoœci zerowane w Init
	int manual_delete;

	// automatycznie ustawiane
	float time, radius;
	vector<Particle> particles;
	int alive;
	bool destroy;

	void Init();
	bool Update(float dt);
	void Save(FileWriter& f);
	void Load(FileReader& f, int version = 2);
	float GetAlpha(const Particle &p) const
	{
		if(op_alpha == POP_CONST)
			return alpha;
		else
			return Lerp(0.f, alpha, p.life / particle_life);
	}
	float GetScale(const Particle &p) const
	{
		if(op_size == POP_CONST)
			return size;
		else
			return Lerp(0.f, size, p.life / particle_life);
	}
};

//-----------------------------------------------------------------------------
struct TrailParticleEmitter : public EntityType<TrailParticleEmitter>
{
	struct Particle
	{
		Vec3 pt;
		float t;
		int next;
		bool exists;
	};

	float fade, timer, width;
	Vec4 color1, color2;
	vector<Particle> parts;
	Texture* tex;
	int first, last, alive;
	bool destroy, manual;

	TrailParticleEmitter() : tex(nullptr), width(0.1f), destroy(false), manual(false) {}
	void Init(int maxp);
	bool Update(float dt);
	void AddPoint(const Vec3& pt);
	void Save(FileWriter& f);
	void Load(FileReader& f, int version = 2);
};

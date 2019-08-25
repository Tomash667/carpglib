#pragma once

//-----------------------------------------------------------------------------
#include "Entity.h"

//-----------------------------------------------------------------------------
struct Particle
{
	Vec3 pos, speed;
	float life, gravity;
	bool exists;
};

//-----------------------------------------------------------------------------
enum PARTICLE_OP
{
	POP_CONST,
	POP_LINEAR_SHRINK
};

//-----------------------------------------------------------------------------
struct ParticleEmitter : public EntityType<ParticleEmitter>
{
	TexturePtr tex;
	float emision_interval, life, particle_life, alpha, size;
	int emisions, spawn_min, spawn_max, max_particles, mode;
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
	void Load(FileReader& f, int version);
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
struct TrailParticle
{
	Vec3 pt1, pt2;
	float t;
	int next;
	bool exists;
};

//-----------------------------------------------------------------------------
struct TrailParticleEmitter : public EntityType<TrailParticleEmitter>
{
	float fade, timer;
	Vec4 color1, color2;
	vector<TrailParticle> parts;
	int first, last, alive;
	bool destroy;

	void Init(int maxp);
	bool Update(float dt, Vec3* pt1, Vec3* pt2);
	void Save(FileWriter& f);
	void Load(FileReader& f, int version);
};

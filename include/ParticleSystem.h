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
	struct Particle
	{
		Vec3 pos, speed;
		float life;
		bool exists;
	};

	TexturePtr tex;
	Vec3 pos, speedMin, speedMax, posMin, posMax;
	Vec2 alpha, size;
	Int2 spawn;
	float emissionInterval, life, particleLife;
	int emissions, maxParticles, mode, manualDelete;
	bool gravity;

	// automatycznie ustawiane
	float time, radius;
	vector<Particle> particles;
	int alive;
	bool destroy;

	ParticleEmitter() : manualDelete(0), gravity(true) {}
	void Init();
	bool Update(float dt);
	void Save(FileWriter& f);
	void Load(FileReader& f, int version = 3);
	float GetAlpha(const Particle &p) const
	{
		return Lerp(alpha.y, alpha.x, p.life / particleLife);
	}
	float GetScale(const Particle &p) const
	{
		return Lerp(size.y, size.x, p.life / particleLife);
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
	void Load(FileReader& f, int version = 3);
};

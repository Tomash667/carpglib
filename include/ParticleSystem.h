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
		float life, gravity;
		bool exists;
	};

	TexturePtr tex;
	Vec3 pos, speedMin, speedMax, posMin, posMax;
	Vec2 alpha, size;
	Int2 spawn;
	float emissionInterval, life, particleLife;
	int emissions, maxParticles, mode;

	// nowe warto�ci, dla kompatybilno�ci zerowane w Init
	int manualDelete;

	// automatycznie ustawiane
	float time, radius;
	vector<Particle> particles;
	int alive;
	bool destroy;

	void Init();
	bool Update(float dt);
	void Save(FileWriter& f);
	void Load(FileReader& f, int version = 3);
	float GetAlpha(const Particle &p) const
	{
		return Lerp(alpha.x, alpha.y, p.life / particleLife);
	}
	float GetScale(const Particle &p) const
	{
		return Lerp(size.x, size.y, p.life / particleLife);
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

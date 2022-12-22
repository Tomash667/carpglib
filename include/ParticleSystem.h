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

struct ParticleEffect
{
	string id;
	TexturePtr tex;
	Vec3 pos, speedMin, speedMax, posMin, posMax;
	Vec2 alpha, size;
	Int2 spawn;
	float emissionInterval, life, particleLife, radius;
	int hash, emissions, maxParticles, mode;
	bool gravity;

	ParticleEffect();
	ParticleEffect(const ParticleEffect& e);
	void CalculateRadius();

	static vector<ParticleEffect*> effects;
	static std::map<int, ParticleEffect*> hashEffects;
	static ParticleEffect* Get(int hash);
	static ParticleEffect* Get(Cstring name) { return Get(Hash(name)); }
};

struct ParticleSystem
{

};

//-----------------------------------------------------------------------------
struct ParticleEmitter : public EntityType<ParticleEmitter>
{
	friend class ParticleShader;

	Vec3 pos;
	int manualDelete;

private:
	struct Particle
	{
		Vec3 pos, speed;
		float life;
		bool exists;
	};

	const ParticleEffect* effect;
	vector<Particle> particles;
	float life, time;
	int emissions, alive;
	bool destroy;

public:
	void Init(const ParticleEffect* effect, const Vec3& pos);
	bool IsAlive() const { return alive; }
	const ParticleEffect* GetEffect() const { return effect; }
	void SetArea(const Box& box);
	void SetTexture(Texture* tex);
	void Destroy() { destroy = true; }
	bool Update(float dt);
	void Save(FileWriter& f);
	void Load(FileReader& f, int version = 3);
	float GetAlpha(const Particle &p) const
	{
		return Lerp(effect->alpha.y, effect->alpha.x, p.life / effect->particleLife);
	}
	float GetScale(const Particle &p) const
	{
		return Lerp(effect->size.y, effect->size.x, p.life / effect->particleLife);
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

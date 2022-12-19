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
	Vec3 pos;
private:
	struct Particle
	{
		Vec3 pos, speed;
		float life, gravity;
		bool exists;
	};

	const ParticleEffect* effect;
	/*TexturePtr tex;
	Vec3 pos, speedMin, speedMax, posMin, posMax;*/
	//PARTICLE_OP opSize, opAlpha;

	float life;
	int emissions;

	// nowe warto�ci, dla kompatybilno�ci zerowane w Init
	int manualDelete;

	// automatycznie ustawiane
	float time;
	vector<Particle> particles;
	int alive;
	bool destroy;

public:
	void Init(const ParticleEffect* effect, const Vec3& pos);
	bool IsAlive() const { return alive; }
	const ParticleEffect* GetEffect() const { return effect; }
	void SetArea(const Box& box);
	void Destroy() { destroy = true; }
	bool Update(float dt);
	void Save(FileWriter& f);
	void Load(FileReader& f, int version = 3);
	/*float GetAlpha(const Particle &p) const
	{
		return Lerp(alpha.x, alpha.y, p.life / particleLife);
	}
	float GetScale(const Particle &p) const
	{
		return Lerp(size.x, size.y, p.life / particleLife);
	}*/
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

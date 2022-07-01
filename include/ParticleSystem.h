#pragma once

//-----------------------------------------------------------------------------
struct Billboard
{
	Texture* tex;
	Vec3 pos;
	float size;
};

//-----------------------------------------------------------------------------
struct ParticleEmitter
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
	Vec3 pos, speedMin, speedMax, posMin, posMax;
	float emissionInterval, life, particleLife, alpha, size;
	int emissions, spawnMin, spawnMax, maxParticles, mode;
	PARTICLE_OP opSize, opAlpha;

	// nowe wartoœci, dla kompatybilnoœci zerowane w Init
	int manualDelete;

	// automatycznie ustawiane
	float time, radius;
	vector<Particle> particles;
	int alive;
	bool destroy;

	void Init();
	bool Update(float dt);
	void Save(FileWriter& f);
	void Load(FileReader& f);
	float GetAlpha(const Particle &p) const
	{
		if(opAlpha == POP_CONST)
			return alpha;
		else
			return Lerp(0.f, alpha, p.life / particleLife);
	}
	float GetScale(const Particle &p) const
	{
		if(opSize == POP_CONST)
			return size;
		else
			return Lerp(0.f, size, p.life / particleLife);
	}
};

//-----------------------------------------------------------------------------
struct TrailParticleEmitter
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
	void Load(FileReader& f);
};

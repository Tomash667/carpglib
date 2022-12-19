#include "Pch.h"
#include "ParticleSystem.h"

#include "File.h"
#include "ResourceManager.h"

bool EntitySystem::clear;
EntityType<ParticleEmitter>::Impl EntityType<ParticleEmitter>::impl;
EntityType<TrailParticleEmitter>::Impl EntityType<TrailParticleEmitter>::impl;

//=================================================================================================
float drop_range(float v, float t)
{
	if(v > 0)
	{
		float t_wznoszenia = v / G;
		if(t_wznoszenia >= t)
			return (v * v) / (2 * G);
		else
			return v * t - (G * (t * t)) / 2;
	}
	else
		return v * t - G * (t * t) / 2;
}

void ParticleEffect::CalculateRadius()
{
	// oblicz promieñ
	float t;
	if(life > 0)
		t = min(particleLife, life);
	else
		t = particleLife;
	float r = 0.f;

	// lewa
	float r2 = abs(posMin.x + speedMin.x * t);
	if(r2 > r)
		r = r2;

	// prawa
	r2 = abs(posMax.x + speedMax.x * t);
	if(r2 > r)
		r = r2;

	// ty³
	r2 = abs(posMin.z + speedMin.z * t);
	if(r2 > r)
		r = r2;

	// przód
	r2 = abs(posMax.z + speedMax.z * t);
	if(r2 > r)
		r = r2;

	// góra
	r2 = abs(posMax.y + drop_range(speedMax.y, t));
	if(r2 > r)
		r = r2;

	// dó³
	r2 = abs(posMin.y + drop_range(speedMin.y, t));
	if(r2 > r)
		r = r2;

	radius = sqrt(2 * r * r);
}

//=================================================================================================
void ParticleEmitter::Init(const ParticleEffect* effect, const Vec3& pos)
{
	assert(effect);
	this->effect = effect;
	this->pos = pos;

	particles.resize(effect->maxParticles);
	life = effect->life;
	time = 0.f;
	emissions = effect->emissions;
	alive = 0;
	destroy = false;
	for(int i = 0; i < effect->maxParticles; ++i)
		particles[i].exists = false;

	// nowe
	manualDelete = 0;
	Register();
}

//=================================================================================================
bool ParticleEmitter::Update(float dt)
{
	if(emissions == 0 || (life > 0 && (life -= dt) <= 0.f))
		destroy = true;

	if(destroy && alive == 0)
	{
		if(manualDelete == 0)
			delete this;
		else
			manualDelete = 2;
		return true;
	}

	// aktualizuj cz¹steczki
	for(vector<Particle>::iterator it2 = particles.begin(), end2 = particles.end(); it2 != end2; ++it2)
	{
		Particle& p = *it2;
		if(!p.exists)
			continue;

		if((p.life -= dt) <= 0.f)
		{
			p.exists = false;
			--alive;
		}
		else
		{
			p.pos += p.speed * dt;
			p.speed.y -= p.gravity * dt;
		}
	}

	// emisja
	if(!destroy && (emissions == -1 || emissions > 0) && ((time += dt) >= effect->emissionInterval))
	{
		if(emissions > 0)
			--emissions;
		time -= effect->emissionInterval;

		int count = min(effect->spawn.Random(), effect->maxParticles - alive);
		vector<Particle>::iterator it2 = particles.begin();

		for(int i = 0; i < count; ++i)
		{
			while(it2->exists)
				++it2;

			Particle& p = *it2;
			p.exists = true;
			p.gravity = G;
			p.life = effect->particleLife;
			p.pos = pos + Vec3::Random(effect->posMin, effect->posMax);
			p.speed = Vec3::Random(effect->speedMin, effect->speedMax);
		}

		alive += count;
	}

	return false;
}

//=================================================================================================
void ParticleEmitter::Save(FileWriter& f)
{
	f << id;
	f << effect->hash;
	f << life;
	f << emissions;
	f << pos;
	f << manualDelete;
	f << time;
	f << particles;
	f << alive;
	f << destroy;
}

//=================================================================================================
void ParticleEmitter::Load(FileReader& f, int version)
{
	if(version >= 3)
	{
		f >> id;
		Register();
		int hash;
		f >> hash;
		f >> life;
		f >> emissions;
		f >> pos;
		f >> manualDelete;
		f >> time;
		f >> particles;
		f >> alive;
		f >> destroy;
	}
	else
	{

		if(version >= 1)
			f >> id;
		Register();

	if(version >= 3)
	{
		tex = app::resMgr->Load<Texture>(f.ReadString1());
		f >> emissionInterval;
		f >> life;
		f >> particleLife;
		f >> alpha;
		f >> size;
		f >> emissions;
		f >> spawn;
		f >> maxParticles;
		f >> mode;
		f >> pos;
		f >> speedMin;
		f >> speedMax;
		f >> posMin;
		f >> posMax;
		f >> manualDelete;
		f >> time;
		f >> radius;
		f >> particles;
		f >> alive;
		f >> destroy;
	}
	else
	{
		float oldAlpha, oldSize;
		int opSize, opAlpha;
		tex = app::resMgr->Load<Texture>(f.ReadString1());
		f >> emissionInterval;
		f >> life;
		f >> particleLife;
		f >> oldAlpha;
		f >> oldSize;
		f >> emissions;
		f >> spawn;
		f >> maxParticles;
		f >> mode;
		f >> pos;
		f >> speedMin;
		f >> speedMax;
		f >> posMin;
		f >> posMax;
		f >> opSize;
		f >> opAlpha;
		f >> manualDelete;
		f >> time;
		f >> radius;
		f >> particles;
		f >> alive;
		f >> destroy;*/

		if(opSize == 0)
			size = Vec2(oldSize);
		else
			size = Vec2(oldSize, 0.f);

		if(opAlpha == 0)
			alpha = Vec2(oldAlpha);
		else
			alpha = Vec2(oldAlpha, 0.f);
	}
}

//=================================================================================================
void TrailParticleEmitter::Init(int maxp)
{
	parts.resize(maxp);
	for(vector<Particle>::iterator it = parts.begin(), end = parts.end(); it != end; ++it)
		it->exists = false;

	first = -1;
	last = -1;
	alive = 0;
	timer = 0.f;
	Register();
}

//=================================================================================================
bool TrailParticleEmitter::Update(float dt)
{
	if(manual)
	{
		if(destroy)
		{
			delete this;
			return true;
		}
		return false;
	}

	if(first != -1)
	{
		int id = first;

		while(id != -1)
		{
			Particle& tp = parts[id];
			tp.t -= dt;
			if(tp.t < 0)
			{
				tp.exists = false;
				first = tp.next;
				--alive;
				if(id == last)
					last = -1;
				id = first;
			}
			else
				id = tp.next;
		}
	}

	timer += dt;

	if(destroy && alive == 0)
	{
		delete this;
		return true;
	}
	else
		return false;
}

//=================================================================================================
void TrailParticleEmitter::AddPoint(const Vec3& pt)
{
	if(timer >= 1.f / parts.size() || first == -1)
	{
		timer = 0.f;

		if(last == -1)
		{
			Particle& tp = parts[0];
			tp.t = fade;
			tp.exists = true;
			tp.next = -1;
			tp.pt = pt;
			first = 0;
			last = 0;
			++alive;
		}
		else
		{
			if(alive == (int)parts.size())
				return;

			int id = 0;
			while(parts[id].exists)
				++id;

			Particle& tp = parts[id];
			tp.t = fade;
			tp.exists = true;
			tp.next = -1;
			tp.pt = pt;

			parts[last].next = id;
			last = id;
			++alive;
		}
	}
}

//=================================================================================================
void TrailParticleEmitter::Save(FileWriter& f)
{
	f << id;
	f << fade;
	f << color1;
	f << color2;
	f << parts;
	f << first;
	f << last;
	f << destroy;
	f << alive;
	f << timer;
	f << width;
	if(tex)
		f << tex->filename;
	else
		f.Write0();
	f << manual;
}

//=================================================================================================
void TrailParticleEmitter::Load(FileReader& f, int version)
{
	if(version >= 1)
		f >> id;
	Register();

	f >> fade;
	f >> color1;
	f >> color2;
	if(version >= 2)
		f >> parts;
	else
	{
		uint count;
		f >> count;
		parts.resize(count);
		for(Particle& p : parts)
		{
			Vec3 pt1, pt2;
			uint exists;
			f >> pt1;
			f >> pt2;
			f >> p.t;
			f >> p.next;
			f >> exists;
			p.exists = (exists & 0xFF) != 0; // saved as 4 bytes due to padding
			if(p.exists)
				p.pt = (pt1 + pt2) / 2;
		}
	}
	f >> first;
	f >> last;
	f >> destroy;
	f >> alive;
	f >> timer;
	if(version >= 2)
	{
		f >> width;
		const string& tex_id = f.ReadString1();
		if(!tex_id.empty())
			tex = app::resMgr->Load<Texture>(tex_id);
		else
			tex = nullptr;
		f >> manual;
	}
	else
	{
		width = 0.1f;
		tex = nullptr;
		manual = false;
	}
}

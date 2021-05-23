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

//=================================================================================================
void ParticleEmitter::Init()
{
	particles.resize(max_particles);
	time = 0.f;
	alive = 0;
	destroy = false;
	for(int i = 0; i < max_particles; ++i)
		particles[i].exists = false;

	// oblicz promie�
	float t;
	if(life > 0)
		t = min(particle_life, life);
	else
		t = particle_life;
	float r = 0.f;

	// lewa
	float r2 = abs(pos_min.x + speed_min.x * t);
	if(r2 > r)
		r = r2;

	// prawa
	r2 = abs(pos_max.x + speed_max.x * t);
	if(r2 > r)
		r = r2;

	// ty�
	r2 = abs(pos_min.z + speed_min.z * t);
	if(r2 > r)
		r = r2;

	// prz�d
	r2 = abs(pos_max.z + speed_max.z * t);
	if(r2 > r)
		r = r2;

	// g�ra
	r2 = abs(pos_max.y + drop_range(speed_max.y, t));
	if(r2 > r)
		r = r2;

	// d�
	r2 = abs(pos_min.y + drop_range(speed_min.y, t));
	if(r2 > r)
		r = r2;

	radius = sqrt(2 * r * r);

	// nowe
	manual_delete = 0;
	Register();
}

//=================================================================================================
bool ParticleEmitter::Update(float dt)
{
	if(emissions == 0 || (life > 0 && (life -= dt) <= 0.f))
		destroy = true;

	if(destroy && alive == 0)
	{
		if(manual_delete == 0)
			delete this;
		else
			manual_delete = 2;
		return true;
	}

	// aktualizuj cz�steczki
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
	if(!destroy && (emissions == -1 || emissions > 0) && ((time += dt) >= emission_interval))
	{
		if(emissions > 0)
			--emissions;
		time -= emission_interval;

		int count = min(Random(spawn_min, spawn_max), max_particles - alive);
		vector<Particle>::iterator it2 = particles.begin();

		for(int i = 0; i < count; ++i)
		{
			while(it2->exists)
				++it2;

			Particle& p = *it2;
			p.exists = true;
			p.gravity = G;
			p.life = particle_life;
			p.pos = pos + Vec3::Random(pos_min, pos_max);
			p.speed = Vec3::Random(speed_min, speed_max);
		}

		alive += count;
	}

	return false;
}

//=================================================================================================
void ParticleEmitter::Save(FileWriter& f)
{
	f << id;
	f << tex->filename;
	f << emission_interval;
	f << life;
	f << particle_life;
	f << alpha;
	f << size;
	f << emissions;
	f << spawn_min;
	f << spawn_max;
	f << max_particles;
	f << mode;
	f << pos;
	f << speed_min;
	f << speed_max;
	f << pos_min;
	f << pos_max;
	f << op_size;
	f << op_alpha;
	f << manual_delete;
	f << time;
	f << radius;
	f << particles;
	f << alive;
	f << destroy;
}

//=================================================================================================
void ParticleEmitter::Load(FileReader& f, int version)
{
	if(version >= 1)
		f >> id;
	Register();

	tex = app::res_mgr->Load<Texture>(f.ReadString1());
	f >> emission_interval;
	f >> life;
	f >> particle_life;
	f >> alpha;
	f >> size;
	f >> emissions;
	f >> spawn_min;
	f >> spawn_max;
	f >> max_particles;
	f >> mode;
	f >> pos;
	f >> speed_min;
	f >> speed_max;
	f >> pos_min;
	f >> pos_max;
	f >> op_size;
	f >> op_alpha;
	f >> manual_delete;
	f >> time;
	f >> radius;
	f >> particles;
	f >> alive;
	f >> destroy;
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
			tex = app::res_mgr->Load<Texture>(tex_id);
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

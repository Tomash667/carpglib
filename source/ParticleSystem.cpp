#include "Pch.h"
#include "ParticleSystem.h"

#include "File.h"
#include "ResourceManager.h"

//=================================================================================================
float DropRange(float v, float t)
{
	if(v > 0)
	{
		float tUp = v / G;
		if(tUp >= t)
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
	particles.resize(maxParticles);
	time = 0.f;
	alive = 0;
	destroy = false;
	for(int i = 0; i < maxParticles; ++i)
		particles[i].exists = false;

	// calculate radius
	radius = 0.f;
	// left
	float r = abs(posMin.x + speedMin.x * particleLife);
	if(r > radius)
		radius = r;
	// right
	r = abs(posMax.x + speedMax.x * particleLife);
	if(r > radius)
		radius = r;
	// back
	r = abs(posMin.z + speedMin.z * particleLife);
	if(r > radius)
		radius = r;
	// front
	r = abs(posMax.z + speedMax.z * particleLife);
	if(r > radius)
		radius = r;
	// up
	if(gravity)
		r = abs(posMax.y + DropRange(speedMax.y, particleLife));
	else
		r = abs(posMax.y + speedMax.y * particleLife);
	if(r > radius)
		radius = r;
	// down
	if(gravity)
		r = abs(posMin.y + DropRange(speedMin.y, particleLife));
	else
		r = abs(posMin.y + speedMin.y * particleLife);
	if(r > radius)
		radius = r;
	radius = sqrt(2 * radius * radius);
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

	// update particles
	if(gravity)
	{
		for(Particle& p : particles)
		{
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
				p.speed.y -= G * dt;
			}
		}
	}
	else
	{
		for(Particle& p : particles)
		{
			if(!p.exists)
				continue;

			if((p.life -= dt) <= 0.f)
			{
				p.exists = false;
				--alive;
			}
			else
				p.pos += p.speed * dt;
		}
	}

	// emission
	if(!destroy && (emissions == -1 || emissions > 0) && ((time += dt) >= emissionInterval))
	{
		if(emissions > 0)
			--emissions;
		time -= emissionInterval;

		int count = min(spawn.Random(), maxParticles - alive);
		vector<Particle>::iterator it2 = particles.begin();

		for(int i = 0; i < count; ++i)
		{
			while(it2->exists)
				++it2;

			Particle& p = *it2;
			p.exists = true;
			p.life = particleLife;
			p.pos = pos + Vec3::Random(posMin, posMax);
			p.speed = Vec3::Random(speedMin, speedMax);
		}

		alive += count;
	}

	return false;
}

//=================================================================================================
void ParticleEmitter::Save(FileWriter& f)
{
	f << tex->filename;
	f << emissionInterval;
	f << life;
	f << particleLife;
	f << alpha;
	f << size;
	f << emissions;
	f << spawn;
	f << maxParticles;
	f << mode;
	f << pos;
	f << speedMin;
	f << speedMax;
	f << posMin;
	f << posMax;
	f << manualDelete;
	f << time;
	f << radius;
	f << particles;
	f << alive;
	f << destroy;
	f << gravity;
}

//=================================================================================================
void ParticleEmitter::Load(FileReader& f)
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
	f >> gravity;
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
void TrailParticleEmitter::Load(FileReader& f)
{
	f >> fade;
	f >> color1;
	f >> color2;
	f >> parts;
	f >> first;
	f >> last;
	f >> destroy;
	f >> alive;
	f >> timer;
	f >> width;
	const string& texId = f.ReadString1();
	if(!texId.empty())
		tex = app::resMgr->Load<Texture>(texId);
	else
		tex = nullptr;
	f >> manual;
}

#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Sound : public Resource
{
	static const ResourceType Type = ResourceType::Sound;

	SOUND sound;

	Sound() : sound(nullptr)
	{
	}
};

//-----------------------------------------------------------------------------
struct Music : public Resource
{
	static const ResourceType Type = ResourceType::Music;

	SOUND sound;

	Music() : sound(nullptr)
	{
	}
};

//-----------------------------------------------------------------------------
struct MusicList
{
	vector<Music*> musics;

	bool IsLoaded() const
	{
		return musics.empty() || musics[0]->IsLoaded();
	}
};

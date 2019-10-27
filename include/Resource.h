#pragma once

//-----------------------------------------------------------------------------
enum class ResourceState
{
	NotLoaded,
	Loading,
	Loaded
};

//-----------------------------------------------------------------------------
enum class ResourceType
{
	Unknown,
	Texture,
	Mesh,
	VertexData,
	Sound,
	Music,
	Font
};

//-----------------------------------------------------------------------------
struct Resource
{
	string path;
	cstring filename;
	ResourceState state;
	ResourceType type;
	Pak* pak;
	uint pak_index;

	virtual ~Resource() {}
	bool IsFile() const { return !pak; }
	bool IsLoaded() const { return state == ResourceState::Loaded; }
	cstring GetPath() const;
	Buffer* GetBuffer();
	void EnsureIsLoaded();
};

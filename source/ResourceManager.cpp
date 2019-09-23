#include "EnginePch.h"
#include "EngineCore.h"
#include "ResourceManager.h"
#include "Engine.h"
#include "Mesh.h"
#include "SoundManager.h"
#include "Pak.h"
#include "Render.h"
#include "DirectX.h"

ResourceManager* app::res_mgr;

//=================================================================================================
ResourceManager::ResourceManager() : mode(Mode::Instant)
{
}

//=================================================================================================
ResourceManager::~ResourceManager()
{
	for(Resource* res : resources)
		delete res;

	for(Pak* pak : paks)
	{
		pak->filename_buf->Free();
		delete pak;
	}

	Buffer::Free(sound_bufs);

	task_pool.Free(tasks);
}

//=================================================================================================
void ResourceManager::Init()
{
	RegisterExtensions();

	Mesh::MeshInit();
}

//=================================================================================================
void ResourceManager::RegisterExtensions()
{
	exts["bmp"] = ResourceType::Texture;
	exts["dds"] = ResourceType::Texture;
	exts["dib"] = ResourceType::Texture;
	exts["hdr"] = ResourceType::Texture;
	exts["jpg"] = ResourceType::Texture;
	exts["pfm"] = ResourceType::Texture;
	exts["png"] = ResourceType::Texture;
	exts["ppm"] = ResourceType::Texture;
	exts["tga"] = ResourceType::Texture;

	exts["qmsh"] = ResourceType::Mesh;

	exts["phy"] = ResourceType::VertexData;

	exts["aiff"] = ResourceType::Sound;
	exts["asf"] = ResourceType::Sound;
	exts["asx"] = ResourceType::Sound;
	exts["dls"] = ResourceType::Sound;
	exts["flac"] = ResourceType::Sound;
	exts["it"] = ResourceType::Sound;
	exts["m3u"] = ResourceType::Sound;
	exts["midi"] = ResourceType::Sound;
	exts["mod"] = ResourceType::Sound;
	exts["mp2"] = ResourceType::Sound;
	exts["mp3"] = ResourceType::Sound;
	exts["pls"] = ResourceType::Sound;
	exts["s3m"] = ResourceType::Sound;
	exts["wav"] = ResourceType::Sound;
	exts["wax"] = ResourceType::Sound;
	exts["wma"] = ResourceType::Sound;
	exts["xm"] = ResourceType::Sound;

	exts["ogg"] = ResourceType::Music;
}

//=================================================================================================
bool ResourceManager::AddDir(cstring dir, bool subdir)
{
	assert(dir);

	int dirlen = strlen(dir) + 1;

	bool ok = io::FindFiles(Format("%s/*.*", dir), [=](const io::FileInfo& file_info)
	{
		if(file_info.is_dir && subdir)
		{
			LocalString path = Format("%s/%s", dir, file_info.filename);
			AddDir(path);
		}
		else
		{
			cstring path = Format("%s/%s", dir, file_info.filename);
			Resource* res = AddResource(file_info.filename, path);
			if(res)
			{
				res->pak = nullptr;
				res->path = path;
				res->filename = res->path.c_str() + dirlen;
			}
		}
		return true;
	});

	if(!ok)
	{
		DWORD result = GetLastError();
		Error("ResourceManager: Failed to add directory '%s' (%u).", dir, result);
		return false;
	}

	return true;
}

//=================================================================================================
bool ResourceManager::AddPak(cstring path, cstring key)
{
	assert(path);

	FileReader f(path);
	if(!f)
	{
		Error("ResourceManager: Failed to open pak '%s' (%u).", path, GetLastError());
		return false;
	}

	// read header
	Pak::Header header;
	f >> header;
	if(!f)
	{
		Error("ResourceManager: Failed to read pak '%s' header.", path);
		return false;
	}
	if(header.sign[0] != 'P' || header.sign[1] != 'A' || header.sign[2] != 'K')
	{
		Error("ResourceManager: Failed to read pak '%s', invalid signature %c%c%c.", path, header.sign[0], header.sign[1], header.sign[2]);
		return false;
	}
	if(header.version != 1)
	{
		Error("ResourceManager: Failed to read pak '%s', invalid version %d.", path, (int)header.version);
		return false;
	}

	uint pak_size = f.GetSize();
	int total_size = pak_size - sizeof(Pak::Header);

	// read table
	if(!f.Ensure(header.file_entry_table_size) || !f.Ensure(header.files_count * sizeof(Pak::File)))
	{
		Error("ResourceManager: Failed to read pak '%s' files table (%u).", path, GetLastError());
		return false;
	}
	Buffer* buf = Buffer::Get();
	buf->Resize(header.file_entry_table_size);
	f.Read(buf->Data(), header.file_entry_table_size);
	total_size -= header.file_entry_table_size;

	// decrypt table
	if(IsSet(header.flags, Pak::Encrypted))
	{
		if(key == nullptr)
		{
			buf->Free();
			Error("ResourceManager: Failed to read pak '%s', file is encrypted.", path);
			return false;
		}
		io::Crypt((char*)buf->Data(), buf->Size(), key, strlen(key));
	}
	if(IsSet(header.flags, Pak::FullEncrypted) && !IsSet(header.flags, Pak::Encrypted))
	{
		buf->Free();
		Error("ResourceManager: Failed to read pak '%s', invalid flags combination %u.", path, header.flags);
		return false;
	}

	// setup pak
	Pak* pak = new Pak;
	pak->encrypted = IsSet(header.flags, Pak::FullEncrypted);
	if(key)
		pak->key = key;
	pak->filename_buf = buf;
	pak->files = (Pak::File*)buf->Data();
	for(uint i = 0; i < header.files_count; ++i)
	{
		Pak::File& file = pak->files[i];
		file.filename = (cstring)buf->Data() + file.filename_offset;
		total_size -= file.compressed_size;

		if(total_size < 0)
		{
			buf->Free();
			Error("ResourceManager: Failed to read pak '%s', broken file size %u at index %u.", path, file.compressed_size, i);
			delete pak;
			return false;
		}

		if(file.offset + file.compressed_size > pak_size)
		{
			buf->Free();
			Error("ResourceManager: Failed to read pak '%s', file at index %u has invalid offset %u (pak size %u).",
				path, i, file.offset, pak_size);
			delete pak;
			return false;
		}

		Resource* res = AddResource(file.filename, path);
		if(res)
		{
			res->pak = pak;
			res->pak_index = i;
			res->filename = file.filename;
		}
	}

	pak->file = f;
	pak->path = path;
	paks.push_back(pak);

	return true;
}

//=================================================================================================
Resource* ResourceManager::AddResource(cstring filename, cstring path)
{
	assert(filename && path);

	ResourceType type = FilenameToResourceType(filename);
	if(type == ResourceType::Unknown)
		return nullptr;

	Resource* res = CreateResource(type);
	res->filename = filename;
	res->type = type;

	pair<ResourceIterator, bool> result = resources.insert(res);
	if(result.second)
	{
		// added
		res->state = ResourceState::NotLoaded;
		return res;
	}
	else
	{
		// already exists
		Resource* existing = *result.first;
		if(existing->pak || existing->path != path)
			Warn("ResourceManager: Resource '%s' already exists (%s; %s).", filename, existing->GetPath(), path);
		delete res;
		return nullptr;
	}
}

//=================================================================================================
void ResourceManager::AddResource(Resource* res)
{
	assert(res);

	pair<ResourceIterator, bool> result = resources.insert(res);
	if(!result.second)
		Warn("ResourceManager: Resource '%s' already added.", res->filename);
}

//=================================================================================================
Resource* ResourceManager::CreateResource(ResourceType type)
{
	switch(type)
	{
	case ResourceType::Texture:
		return new Texture;
	case ResourceType::Mesh:
		return new Mesh;
	case ResourceType::VertexData:
		return new VertexData;
	case ResourceType::Sound:
		return new Sound;
	case ResourceType::Music:
		return new Music;
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
Resource* ResourceManager::TryGetResource(Cstring filename, ResourceType type)
{
	res_search.filename = filename;
	auto it = resources.find(&res_search);
	if(it != resources.end())
	{
		assert((*it)->type == type);
		return *it;
	}
	else
		return nullptr;
}

//=================================================================================================
Resource* ResourceManager::GetResource(Cstring filename, ResourceType type)
{
	Resource* res = TryGetResource(filename, type);
	if(!res)
	{
		cstring type_name;
		switch(type)
		{
		case ResourceType::Mesh:
			type_name = "mesh";
			break;
		case ResourceType::Texture:
			type_name = "texture";
			break;
		case ResourceType::VertexData:
			type_name = "vertex data";
			break;
		case ResourceType::Sound:
			type_name = "sound";
			break;
		case ResourceType::Music:
			type_name = "music";
			break;
		default:
			assert(0);
			type_name = "unknown";
			break;
		}
		throw Format("ResourceManager: Missing %s '%s'.", type_name, filename);
	}
	return res;
}

//=================================================================================================
void ResourceManager::Load(Resource* res)
{
	assert(res);
	if(res->state == ResourceState::Loaded)
		return;
	if(mode != Mode::LoadScreenPrepare)
		LoadResourceInternal(res);
	else if(res->state == ResourceState::NotLoaded)
	{
		TaskDetail* td = task_pool.Get();
		td->data.res = res;
		td->type = TaskType::Load;
		tasks.push_back(td);
		++to_load;

		res->state = ResourceState::Loading;
	}
}

//=================================================================================================
void ResourceManager::LoadInstant(Resource* res)
{
	assert(res);
	if(res->state != ResourceState::Loaded)
		LoadResourceInternal(res);
}

//=================================================================================================
ResourceType ResourceManager::ExtToResourceType(cstring ext)
{
	auto it = exts.find(ext);
	if(it != exts.end())
		return it->second;
	else
		return ResourceType::Unknown;
}

//=================================================================================================
ResourceType ResourceManager::FilenameToResourceType(cstring filename)
{
	cstring pos = strrchr(filename, '.');
	if(pos == nullptr || !(*pos + 1))
		return ResourceType::Unknown;
	else
		return ExtToResourceType(pos + 1);
}

//=================================================================================================
void ResourceManager::AddTaskCategory(Cstring category)
{
	assert(mode == Mode::LoadScreenPrepare);

	TaskDetail* td = task_pool.Get();
	td->category = category;
	td->type = TaskType::Category;
	tasks.push_back(td);
}

//=================================================================================================
void ResourceManager::AddTask(void* ptr, TaskCallback callback)
{
	assert(mode == Mode::LoadScreenPrepare);

	TaskDetail* td = task_pool.Get();
	td->callback = callback;
	td->data.ptr = ptr;
	td->type = TaskType::Callback;
	tasks.push_back(td);
	++to_load;
}

//=================================================================================================
void ResourceManager::PrepareLoadScreen(float progress_min, float progress_max)
{
	assert(mode == Mode::Instant && InRange(progress_min, 0.f, 1.f) && InRange(progress_max, 0.f, 1.f) && progress_max >= progress_min);

	this->progress_min = progress_min;
	this->progress_max = progress_max;
	progress = progress_min;
	to_load = 0;
	loaded = 0;
	mode = Mode::LoadScreenPrepare;
	category = nullptr;
}

//=================================================================================================
void ResourceManager::StartLoadScreen(cstring category)
{
	assert(mode == Mode::LoadScreenPrepare);

	mode = Mode::LoadScreenRuning;
	if(category)
		this->category = category;
	UpdateLoadScreen();
	mode = Mode::Instant;
	task_pool.Free(tasks);
}

//=================================================================================================
void ResourceManager::CancelLoadScreen(bool cleanup)
{
	assert(mode == Mode::LoadScreenPrepare);

	if(cleanup)
	{
		for(TaskDetail* task : tasks)
		{
			if(task->type == TaskType::Load && task->data.res->state == ResourceState::Loading)
				task->data.res->state = ResourceState::NotLoaded;
		}
		task_pool.Free(tasks);
	}
	else
		assert(tasks.empty());

	mode = Mode::Instant;
}

//=================================================================================================
void ResourceManager::UpdateLoadScreen()
{
	if(to_load == loaded)
	{
		// no tasks to load, draw with full progress
		progress = progress_max;
		progress_clbk(progress_max, "");
		app::engine->DoPseudotick();
		return;
	}

	// draw first frame
	if(tasks[0]->type == TaskType::Category)
		category = tasks[0]->category;
	progress_clbk(progress, category);
	app::engine->DoPseudotick();

	// do all tasks
	timer.Reset();
	timer_dt = 0;
	for(uint i = 0; i < tasks.size(); ++i)
	{
		TaskDetail* task = tasks[i];
		switch(task->type)
		{
		case TaskType::Category:
			category = task->category;
			break;
		case TaskType::Callback:
			task->callback(task->data);
			++loaded;
			break;
		case TaskType::Load:
			if(task->data.res->state != ResourceState::Loaded)
				LoadResourceInternal(task->data.res);
			++loaded;
			break;
		default:
			assert(0);
			break;
		}

		TickLoadScreen();
	}

	// draw last frame
	progress = progress_max;
	progress_clbk(progress_max, nullptr);
	app::engine->DoPseudotick();
}

//=================================================================================================
void ResourceManager::TickLoadScreen()
{
	timer_dt += timer.Tick();
	if(timer_dt >= 0.05f)
	{
		timer_dt = 0.f;
		progress = float(loaded) / to_load * (progress_max - progress_min) + progress_min;
		progress_clbk(progress, category);
		app::engine->DoPseudotick();
	}
}

//=================================================================================================
void ResourceManager::LoadResourceInternal(Resource* res)
{
	assert(res->state != ResourceState::Loaded);

	switch(res->type)
	{
	case ResourceType::Mesh:
		LoadMesh(static_cast<Mesh*>(res));
		break;
	case ResourceType::VertexData:
		LoadVertexData(static_cast<VertexData*>(res));
		break;
	case ResourceType::Sound:
	case ResourceType::Music:
		LoadSoundOrMusic(static_cast<Sound*>(res));
		break;
	case ResourceType::Texture:
		LoadTexture(static_cast<Texture*>(res));
		break;
	default:
		assert(0);
		break;
	}

	res->state = ResourceState::Loaded;
}

//=================================================================================================
void ResourceManager::LoadMesh(Mesh* mesh)
{
	try
	{
		IDirect3DDevice9* device = app::render->GetDevice();
		if(mesh->IsFile())
		{
			FileReader f(mesh->path);
			mesh->Load(f, device);
		}
		else
		{
			MemoryReader f(mesh->GetBuffer());
			mesh->Load(f, device);
		}
	}
	catch(cstring err)
	{
		throw Format("ResourceManager: Failed to load mesh '%s'. %s", mesh->GetPath(), err);
	}
}

//=================================================================================================
void ResourceManager::LoadMeshMetadata(Mesh* mesh)
{
	try
	{
		if(mesh->IsFile())
		{
			FileReader f(mesh->path);
			mesh->LoadMetadata(f);
		}
		else
		{
			MemoryReader f(mesh->GetBuffer());
			mesh->LoadMetadata(f);
		}
	}
	catch(cstring err)
	{
		throw Format("ResourceManager: Failed to load mesh metadata '%s'. %s", mesh->GetPath(), err);
	}
}

//=================================================================================================
void ResourceManager::LoadVertexData(VertexData* vd)
{
	try
	{
		if(vd->IsFile())
		{
			FileReader f(vd->path);
			Mesh::LoadVertexData(vd, f);
		}
		else
		{
			MemoryReader f(vd->GetBuffer());
			Mesh::LoadVertexData(vd, f);
		}
	}
	catch(cstring err)
	{
		throw Format("ResourceManager: Failed to load mesh vertex data '%s'. %s", vd->GetPath(), err);
	}
}

//=================================================================================================
void ResourceManager::LoadSoundOrMusic(Sound* sound)
{
	int result = app::sound_mgr->LoadSound(sound);
	if(result != 0)
		throw Format("ResourceManager: Failed to load %s '%s' (%d).", sound->type == ResourceType::Music ? "music" : "sound", sound->path.c_str(), result);
}

//=================================================================================================
void ResourceManager::LoadTexture(Texture* tex)
{
	IDirect3DDevice9* device = app::render->GetDevice();
	HRESULT hr;

	if(tex->IsFile())
	{
		hr = D3DXCreateTextureFromFile(device, tex->path.c_str(), &tex->tex);
		if(FAILED(hr))
		{
			Sleep(250);
			hr = D3DXCreateTextureFromFile(device, tex->path.c_str(), &tex->tex);
		}
	}
	else
	{
		BufferHandle&& buf = tex->GetBuffer();
		hr = D3DXCreateTextureFromFileInMemory(device, buf->Data(), buf->Size(), &tex->tex);
	}

	if(FAILED(hr))
		throw Format("Failed to load texture '%s' (%u).", tex->GetPath(), hr);
}

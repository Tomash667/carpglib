#pragma once

//-----------------------------------------------------------------------------
#include "Texture.h"
#include "Sound.h"
#include "Timer.h"

//-----------------------------------------------------------------------------
// Task data
struct TaskData
{
	Resource* res;
	void* ptr;

	TaskData(void* ptr = nullptr) : ptr(ptr), res(nullptr) {}
};

//-----------------------------------------------------------------------------
typedef delegate<void(float, cstring)> ProgressCallback;
typedef delegate<void(TaskData&)> TaskCallback;

//-----------------------------------------------------------------------------
// Task
struct Task : TaskData
{
	enum Flags
	{
		Assign = 1 << 0
	};

	TaskCallback callback;
	int flags;

	Task() : flags(0) {}
	Task(void* ptr) : TaskData(ptr), flags(Assign) {}
	Task(void* ptr, TaskCallback& callback) : TaskData(ptr), callback(callback), flags(0) {}
};

//-----------------------------------------------------------------------------
template<typename T>
struct PtrOrRef
{
	PtrOrRef(T* ptr) : ptr(ptr) {}
	PtrOrRef(T& ref) : ptr(&ref) {}
	T* ptr;
};

//-----------------------------------------------------------------------------
class ResourceManager
{
	enum class Mode
	{
		Instant,
		LoadScreenPrepare,
		LoadScreenRuning,
		LoadScreenRuningPrepareNext
	};

	enum class TaskType
	{
		Callback,
		Load,
		Category
	};

	struct ResourceComparer
	{
		bool operator () (const Resource* r1, const Resource* r2) const
		{
			return _stricmp(r1->filename, r2->filename) > 0;
		}
	};

	typedef std::set<Resource*, ResourceComparer> ResourceContainer;
	typedef ResourceContainer::iterator ResourceIterator;

	public:
	ResourceManager();
	~ResourceManager();

	void Init();
	bool AddDir(cstring dir, bool subdir = true);
	bool AddPak(cstring path, cstring key = nullptr);
	void AddResource(Resource* res);
	ResourceType ExtToResourceType(cstring ext);
	ResourceType FilenameToResourceType(cstring filename);
	void AddTaskCategory(Cstring name);
	void AddTask(void* ptr, TaskCallback callback);
	void SetProgressCallback(ProgressCallback clbk) { progress_clbk = clbk; }
	void PrepareLoadScreen(float progress_min = 0.f, float progress_max = 1.f);
	void StartLoadScreen(cstring category = nullptr);
	void CancelLoadScreen(bool cleanup = false);
	bool HaveTasks() const { return !tasks.empty(); }
	int GetLoadTasksCount() const { return to_load; }
	bool IsLoadScreen() const { return mode != Mode::Instant; }

	// Return resource or null if missing
	template<typename T>
	T* TryGet(Cstring filename)
	{
		return static_cast<T*>(TryGetResource(filename, T::Type));
	}
	// Return resource or throw if missing
	template<typename T>
	T* Get(Cstring filename)
	{
		return static_cast<T*>(GetResource(filename, T::Type));
	}
	// Return resource (load it now or in background depending on mode) or null if missing
	template<typename T>
	T* TryLoad(Cstring filename)
	{
		T* res = TryGet<T>(filename);
		if(res)
			Load(res);
		return res;
	}
	// Return resource (load it now or in background depending on mode) or throw if missing
	template<typename T>
	T* Load(Cstring filename)
	{
		T* res = Get<T>(filename);
		Load(res);
		return res;
	}
	void Load(Resource* res);
	// Return loaded resource or throw if missing
	template<typename T>
	T* LoadInstant(Cstring filename)
	{
		T* res = Get<T>(filename);
		LoadInstant(res);
		return res;
	}
	template<typename T>
	T* TryLoadInstant(Cstring filename)
	{
		T* res = TryGet<T>(filename);
		if(res)
			LoadInstant(res);
		return res;
	}
	void LoadInstant(Resource* res);
	void LoadMeshMetadata(Mesh* mesh);

private:
	struct TaskDetail
	{
		union
		{
			TaskData data;
			cstring category;
		};
		TaskType type;
		TaskCallback callback;

		TaskDetail()
		{
		}
	};

	void RegisterExtensions();
	void UpdateLoadScreen();
	void TickLoadScreen();

	Resource* AddResource(cstring filename, cstring path);
	Resource* CreateResource(ResourceType type);
	Resource* TryGetResource(Cstring filename, ResourceType type);
	Resource* GetResource(Cstring filename, ResourceType type);
	void LoadResourceInternal(Resource* res);
	void LoadMesh(Mesh* mesh);
	void LoadVertexData(VertexData* vd);
	void LoadSoundOrMusic(Sound* sound);
	void LoadTexture(Texture* tex);

	Mode mode;
	ResourceContainer resources;
	Resource res_search;
	std::map<cstring, ResourceType, CstringComparer> exts;
	vector<Pak*> paks;
	vector<Buffer*> sound_bufs;
	vector<TaskDetail*> tasks;
	int to_load, loaded;
	cstring category;
	Timer timer;
	float timer_dt, progress, progress_min, progress_max;
	ProgressCallback progress_clbk;
	ObjectPool<TaskDetail> task_pool;
};

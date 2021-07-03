#include "Pch.h"
#include "SoundManager.h"

#include "ResourceManager.h"
#define INCLUDE_DEVICE_API
#include "WindowsIncludes.h"

#include <fmod.hpp>

SoundManager* app::sound_mgr;

class DefaultDeviceHandler : public IMMNotificationClient
{
public:
	DefaultDeviceHandler(IMMDeviceEnumerator* enumerator) : enumerator(enumerator)
	{
		enumerator->RegisterEndpointNotificationCallback(this);
	}

	virtual ~DefaultDeviceHandler()
	{
		enumerator->UnregisterEndpointNotificationCallback(this);
		enumerator->Release();
	}

	HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR)
	{
		if(flow == EDataFlow::eRender && role == ERole::eConsole)
			app::sound_mgr->HandleDefaultDeviceChange(GetDefaultDevice());
		return S_OK;
	}

	Guid GetDefaultDevice()
	{
		IMMDevice* device;
		HRESULT hr = enumerator->GetDefaultAudioEndpoint(EDataFlow::eRender, ERole::eConsole, &device);
		assert(SUCCEEDED(hr));

		IPropertyStore* props;
		hr = device->OpenPropertyStore(STGM_READ, &props);
		assert(SUCCEEDED(hr));

		PROPVARIANT prop;
		PropVariantInit(&prop);
		hr = props->GetValue(PKEY_AudioEndpoint_GUID, &prop);
		assert(SUCCEEDED(hr));

		CLSID guid;
		Guid deviceGuid;
		CLSIDFromString(prop.pwszVal, &guid);
		memcpy(&deviceGuid, &guid, sizeof(Guid));

		PropVariantClear(&prop);
		props->Release();
		device->Release();
		return deviceGuid;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface)
	{
		if(IID_IUnknown == riid)
			*ppvInterface = (IUnknown*)this;
		else if(__uuidof(IMMNotificationClient) == riid)
			*ppvInterface = (IMMNotificationClient*)this;
		else
		{
			*ppvInterface = nullptr;
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	// not implemented
	HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) { return S_OK; }
	HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) { return S_OK; }
	HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) { return S_OK; }
	HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) { return S_OK; }
	ULONG STDMETHODCALLTYPE AddRef() { return 1; }
	ULONG STDMETHODCALLTYPE Release() { return 0; }

private:
	IMMDeviceEnumerator* enumerator;
};

//=================================================================================================
SoundManager::SoundManager() : system(nullptr), handler(nullptr), device(Guid::Empty), newDevice(Guid::Empty), currentMusic(nullptr), musicEnded(false),
disabled(false), soundVolume(50), musicVolume(50)
{
}

//=================================================================================================
SoundManager::~SoundManager()
{
	delete handler;
	criticalSection.Free();
	if(system)
		system->release();
}

//=================================================================================================
void SoundManager::Init()
{
	// if disabled, log it
	if(disabled)
	{
		Info("SoundMgr: Sound and music is disabled.");
		playSound = false;
		return;
	}

	// create FMOD system
	FMOD_RESULT result = FMOD::System_Create(&system);
	if(result != FMOD_OK)
		throw Format("SoundMgr: Failed to create FMOD system (%d).", result);

	// get number of devices
	int count;
	result = system->getNumDrivers(&count);
	if(result != FMOD_OK)
		throw Format("SoundMgr: Failed to get number of devices (%d).", result);
	if(count == 0)
	{
		Warn("SoundMgr: No sound devices found.");
		disabled = true;
		playSound = false;
		return;
	}

	// log devices
	Info("SoundMgr: Sound devices (%d):", count);
	LocalString str;
	str->resize(256);
	bool deviceOk = false;
	for(int i = 0; i < count; ++i)
	{
		Guid guid;
		result = system->getDriverInfo(i, str.data(), 256, reinterpret_cast<FMOD_GUID*>(&guid), nullptr, nullptr, nullptr);
		if(result == FMOD_OK)
		{
			Utf8ToAscii(str);
			Info("SoundMgr: Device %d - %s", i, str->c_str());
			if(guid == device)
			{
				deviceOk = true;
				system->setDriver(i);
			}
		}
		else
			Error("SoundMgr: Failed to get device %d info (%d).", i, result);
	}

	// get info about selected device and output device
	int selectedDriver;
	FMOD_OUTPUTTYPE output;
	system->getDriver(&selectedDriver);
	system->getOutput(&output);
	Info("SoundMgr: Using device %d and output type %d.", selectedDriver, output);
	if(!deviceOk && device != Guid::Empty)
	{
		Warn("SoundMgr: Selected device not found, using system default.");
		device = Guid::Empty;
	}

	// initialize FMOD system
	bool ok = false;
	for(int i = 0; i < 3; ++i)
	{
		result = system->init(128, FMOD_INIT_NORMAL, nullptr);
		if(result != FMOD_OK)
		{
			Error("SoundMgr: Failed to initialize FMOD system (%d).", result);
			Sleep(100);
		}
		else
		{
			ok = true;
			break;
		}
	}
	if(!ok)
	{
		Error("SoundMgr: Failed to initialize FMOD, disabling sound!");
		disabled = true;
		playSound = false;
		return;
	}

	// create group for sounds and music
	system->createChannelGroup("default", &groupDefault);
	system->createChannelGroup("music", &groupMusic);
	groupDefault->setVolume(float(soundVolume) / 100);
	groupMusic->setVolume(float(musicVolume) / 100);
	playSound = soundVolume > 0;

	// register change default device handler
	IMMDeviceEnumerator* enumerator;
	[[maybe_unused]] HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
	assert(SUCCEEDED(hr));
	criticalSection.Create();
	handler = new DefaultDeviceHandler(enumerator);
	defaultDevice = handler->GetDefaultDevice();

	Info("SoundMgr: FMOD sound system created.");
}

//=================================================================================================
void SoundManager::Update(float dt)
{
	if(disabled)
		return;

	LoopAndRemove(fallbacks, [dt](FMOD::Channel* channel)
	{
		float volume;
		channel->getVolume(&volume);
		if((volume -= dt) <= 0.f)
		{
			channel->stop();
			return true;
		}
		else
		{
			channel->setVolume(volume);
			return false;
		}
	});

	if(currentMusic)
	{
		float volume;
		currentMusic->getVolume(&volume);
		if(volume != 1.f)
		{
			volume = min(1.f, volume + dt);
			currentMusic->setVolume(volume);
		}

		bool playing;
		currentMusic->isPlaying(&playing);
		if(!playing)
		{
			if(musicList)
			{
				if(musics.empty())
					SetupMusics();

				lastMusic = musics.back();
				system->playSound(lastMusic->sound, groupMusic, true, &currentMusic);
				currentMusic->setVolume(0.f);
				currentMusic->setPaused(false);
				musics.pop_back();
			}
			else
			{
				musicEnded = true;
				currentMusic = nullptr;
			}
		}
	}

	system->update();

	if(handler)
	{
		Guid changeDevice;
		criticalSection.Enter();
		changeDevice = newDevice;
		newDevice = Guid::Empty;
		criticalSection.Leave();
		if(changeDevice != Guid::Empty)
		{
			defaultDevice = changeDevice;

			if(device == Guid::Empty)
			{
				FMOD_OUTPUTTYPE output;
				system->getOutput(&output);
				system->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
				system->setOutput(output);

				int count;
				FMOD_RESULT result = system->getNumDrivers(&count);
				assert(result == FMOD_OK && count > 0);

				LocalString str;
				str->resize(256);
				for(int i = 0; i < count; ++i)
				{
					Guid guid;
					result = system->getDriverInfo(i, str.data(), 256, reinterpret_cast<FMOD_GUID*>(&guid), nullptr, nullptr, nullptr);
					if(result == FMOD_OK)
					{
						if(guid == defaultDevice)
						{
							Utf8ToAscii(str);
							Info("SoundMgr: Default device changed to '%s'.", str->c_str());
							system->setDriver(i);
							return;
						}
					}
				}
				Error("SoundMgr: Default device change to invalid device '%s'.", defaultDevice.ToString());
			}
		}
	}
}

//=================================================================================================
int SoundManager::LoadSound(Sound* sound)
{
	assert(sound);

	if(!system)
		return 0;

	int flags = FMOD_LOWMEM;
	if(sound->type == ResourceType::Music)
		flags |= FMOD_2D;
	else
		flags |= FMOD_3D | FMOD_LOOP_OFF;

	FMOD_RESULT result;
	if(sound->IsFile())
		result = system->createStream(sound->path.c_str(), flags, nullptr, &sound->sound);
	else
	{
		BufferHandle&& buf = sound->GetBuffer();
		FMOD_CREATESOUNDEXINFO info = { 0 };
		info.cbsize = sizeof(info);
		info.length = buf->Size();
		result = system->createStream((cstring)buf->Data(), flags | FMOD_OPENMEMORY, &info, &sound->sound);
		if(result == FMOD_OK)
			soundBufs.push_back(buf.Pin());
	}

	return (int)result;
}

//=================================================================================================
void SoundManager::PlayMusic(Music* music)
{
	if(disabled)
		return;

	assert(music && music->IsLoaded());

	if(musicList == nullptr && music == lastMusic)
		return;

	musicList = nullptr;
	musicEnded = false;
	if(currentMusic)
		fallbacks.push_back(currentMusic);

	system->playSound(music->sound, groupMusic, true, &currentMusic);
	currentMusic->setVolume(0.f);
	currentMusic->setPaused(false);
}

//=================================================================================================
void SoundManager::PlayMusic(MusicList* musicList, bool delayed)
{
	if(disabled)
		return;

	assert(musicList && musicList->IsLoaded());

	if(this->musicList == musicList)
		return;

	musicEnded = false;
	this->musicList = musicList;
	lastMusic = nullptr;
	SetupMusics();

	if(!delayed || !currentMusic)
	{
		if(currentMusic)
			fallbacks.push_back(currentMusic);

		if(!musics.empty())
		{
			lastMusic = musics.back();
			system->playSound(lastMusic->sound, groupMusic, true, &currentMusic);
			currentMusic->setVolume(0.f);
			currentMusic->setPaused(false);
			musics.pop_back();
		}
		else
			currentMusic = nullptr;
	}
}

//=================================================================================================
void SoundManager::PlaySound2d(Sound* sound)
{
	assert(sound);

	if(!playSound)
		return;

	assert(sound->IsLoaded());

	FMOD::Channel* channel;
	system->playSound(sound->sound, groupDefault, true, &channel);
	channel->setMode(FMOD_2D | FMOD_LOWMEM | FMOD_LOOP_OFF);
	channel->setPaused(false);
	playingSounds.push_back(channel);
}

//=================================================================================================
void SoundManager::PlaySound3d(Sound* sound, const Vec3& pos, float distance)
{
	assert(sound);

	if(!playSound)
		return;

	assert(sound->IsLoaded());

	FMOD::Channel* channel;
	system->playSound(sound->sound, groupDefault, true, &channel);
	channel->setMode(FMOD_3D | FMOD_LOWMEM | FMOD_LOOP_OFF);
	channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
	channel->set3DMinMaxDistance(distance, 10000.f);
	channel->setPaused(false);
	playingSounds.push_back(channel);
}

//=================================================================================================
FMOD::Channel* SoundManager::CreateChannel(Sound* sound, const Vec3& pos, float distance)
{
	assert(playSound && sound);
	assert(sound->IsLoaded());

	FMOD::Channel* channel;
	system->playSound(sound->sound, groupDefault, true, &channel);
	channel->setMode(FMOD_3D | FMOD_LOWMEM | FMOD_LOOP_OFF);
	channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
	channel->set3DMinMaxDistance(distance, 10000.f);
	channel->setPaused(false);
	playingSounds.push_back(channel);
	return channel;
}

//=================================================================================================
void SoundManager::StopSounds()
{
	if(disabled)
		return;
	for(vector<FMOD::Channel*>::iterator it = playingSounds.begin(), end = playingSounds.end(); it != end; ++it)
		(*it)->stop();
	playingSounds.clear();
}

//=================================================================================================
void SoundManager::StopMusic()
{
	if(currentMusic)
	{
		currentMusic->stop();
		currentMusic = nullptr;
	}
	lastMusic = nullptr;
	musicList = nullptr;
}

//=================================================================================================
void SoundManager::SetListenerPosition(const Vec3& pos, const Vec3& dir, const Vec3& up)
{
	if(disabled)
		return;
	system->set3DListenerAttributes(0, (const FMOD_VECTOR*)&pos, nullptr, (const FMOD_VECTOR*)&dir, (const FMOD_VECTOR*)&up);
}

//=================================================================================================
void SoundManager::SetSoundVolume(int volume)
{
	assert(InRange(volume, 0, 100));
	soundVolume = volume;
	if(!disabled)
	{
		groupDefault->setVolume(float(soundVolume) / 100);
		playSound = soundVolume > 0;
	}
}

//=================================================================================================
void SoundManager::SetMusicVolume(int volume)
{
	assert(InRange(volume, 0, 100));
	musicVolume = volume;
	if(!disabled)
		groupMusic->setVolume(float(musicVolume) / 100);
}

//=================================================================================================
bool SoundManager::UpdateChannelPosition(FMOD::Channel* channel, const Vec3& pos)
{
	assert(channel);

	bool isPlaying;
	if(channel->isPlaying(&isPlaying) == FMOD_OK && isPlaying)
	{
		channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
		return true;
	}
	else
		return false;
}

//=================================================================================================
bool SoundManager::IsPlaying(FMOD::Channel* channel)
{
	assert(channel);
	bool isPlaying;
	if(channel->isPlaying(&isPlaying) == FMOD_OK)
		return isPlaying;
	else
		return false;
}

//=================================================================================================
void SoundManager::SetDevice(Guid _device)
{
	if(device == _device)
		return;
	device = _device;
	if(system)
	{
		int count;
		FMOD_RESULT result = system->getNumDrivers(&count);
		assert(result == FMOD_OK && count > 0);

		LocalString str;
		str->resize(256);
		for(int i = 0; i < count; ++i)
		{
			Guid guid;
			result = system->getDriverInfo(i, str.data(), 256, reinterpret_cast<FMOD_GUID*>(&guid), nullptr, nullptr, nullptr);
			if(result == FMOD_OK)
			{
				if(guid == device || (device == Guid::Empty && guid == defaultDevice))
				{
					Utf8ToAscii(str);
					if(device == Guid::Empty)
						Info("SoundMgr: Changed device to default '%s'.", str->c_str());
					else
						Info("SoundMgr: Changed device to '%s'.", str->c_str());
					system->setDriver(i);
					return;
				}
			}
		}
		Error("SoundMgr: Invalid device id '%s'.", device.ToString());
	}
}

//=================================================================================================
void SoundManager::GetDevices(vector<pair<Guid, string>>& devices) const
{
	devices.clear();

	int count;
	FMOD_RESULT result = system->getNumDrivers(&count);
	assert(result == FMOD_OK && count > 0);

	devices.resize(count);
	LocalString str;
	str->resize(256);
	for(int i = 0; i < count; ++i)
	{
		Guid guid;
		result = system->getDriverInfo(i, str.data(), 256, reinterpret_cast<FMOD_GUID*>(&guid), nullptr, nullptr, nullptr);
		if(result == FMOD_OK)
		{
			Utf8ToAscii(str);
			devices[i].first = guid;
			devices[i].second = (string&)str;
		}
	}
}

//=================================================================================================
void SoundManager::HandleDefaultDeviceChange(const Guid& device)
{
	criticalSection.Enter();
	newDevice = device;
	criticalSection.Leave();
}

//=================================================================================================
void SoundManager::SetupMusics()
{
	musics = musicList->musics;
	if(musics.size() != 1u)
	{
		Shuffle(musics.begin(), musics.end());
		if(musics.back() == lastMusic)
			std::iter_swap(musics.begin(), musics.end() - 1);
	}
}

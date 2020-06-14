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
SoundManager::SoundManager() : system(nullptr), handler(nullptr), device(Guid::Empty), newDevice(Guid::Empty), current_music(nullptr), music_ended(false),
nosound(false), nomusic(false), sound_volume(50), music_volume(50)
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
	disabled_sound = nosound && nomusic;
	play_sound = !nosound && sound_volume > 0;

	// if disabled, log it
	if(disabled_sound)
	{
		Info("SoundMgr: Sound and music is disabled.");
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
		disabled_sound = true;
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
		disabled_sound = true;
		return;
	}

	// create group for sounds and music
	system->createChannelGroup("default", &group_default);
	system->createChannelGroup("music", &group_music);
	group_default->setVolume(float(nosound ? 0 : sound_volume) / 100);
	group_music->setVolume(float(nomusic ? 0 : music_volume) / 100);

	// register change default device handler
	IMMDeviceEnumerator* enumerator;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
	assert(SUCCEEDED(hr));
	criticalSection.Create();
	handler = new DefaultDeviceHandler(enumerator);
	defaultDevice = handler->GetDefaultDevice();

	Info("SoundMgr: FMOD sound system created.");
}

//=================================================================================================
void SoundManager::Disable(bool nosound, bool nomusic)
{
	assert(!initialized);
	this->nosound = nosound;
	this->nomusic = nomusic;
}

//=================================================================================================
void SoundManager::Update(float dt)
{
	if(disabled_sound)
		return;

	bool deletions = false;
	float volume;

	for(vector<FMOD::Channel*>::iterator it = fallbacks.begin(), end = fallbacks.end(); it != end; ++it)
	{
		(*it)->getVolume(&volume);
		if((volume -= dt) <= 0.f)
		{
			(*it)->stop();
			*it = nullptr;
			deletions = true;
		}
		else
			(*it)->setVolume(volume);
	}

	if(deletions)
		RemoveNullElements(fallbacks);

	if(current_music)
	{
		current_music->getVolume(&volume);
		if(volume != 1.f)
		{
			volume = min(1.f, volume + dt);
			current_music->setVolume(volume);
		}

		bool playing;
		current_music->isPlaying(&playing);
		if(!playing)
			music_ended = true;
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
			sound_bufs.push_back(buf.Pin());
	}

	return (int)result;
}

//=================================================================================================
void SoundManager::PlayMusic(Music* music)
{
	if(nomusic || (!music && !current_music))
	{
		music_ended = true;
		return;
	}

	assert(!music || music->IsLoaded());

	music_ended = false;
	if(music && current_music)
	{
		FMOD::Sound* music_sound;
		current_music->getCurrentSound(&music_sound);

		if(music_sound == music->sound)
			return;
	}

	if(current_music)
		fallbacks.push_back(current_music);

	if(music)
	{
		system->playSound(music->sound, group_music, true, &current_music);
		current_music->setVolume(0.f);
		current_music->setPaused(false);
	}
	else
		current_music = nullptr;
}

//=================================================================================================
void SoundManager::PlaySound2d(Sound* sound)
{
	assert(sound);

	if(!play_sound)
		return;

	assert(sound->IsLoaded());

	FMOD::Channel* channel;
	system->playSound(sound->sound, group_default, true, &channel);
	channel->setMode(FMOD_2D | FMOD_LOWMEM | FMOD_LOOP_OFF);
	channel->setPaused(false);
	playing_sounds.push_back(channel);
}

//=================================================================================================
void SoundManager::PlaySound3d(Sound* sound, const Vec3& pos, float distance)
{
	assert(sound);

	if(!play_sound)
		return;

	assert(sound->IsLoaded());

	FMOD::Channel* channel;
	system->playSound(sound->sound, group_default, true, &channel);
	channel->setMode(FMOD_3D | FMOD_LOWMEM | FMOD_LOOP_OFF);
	channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
	channel->set3DMinMaxDistance(distance, 10000.f);
	channel->setPaused(false);
	playing_sounds.push_back(channel);
}

//=================================================================================================
FMOD::Channel* SoundManager::CreateChannel(Sound* sound, const Vec3& pos, float distance)
{
	assert(play_sound && sound);
	assert(sound->IsLoaded());

	FMOD::Channel* channel;
	system->playSound(sound->sound, group_default, true, &channel);
	channel->setMode(FMOD_3D | FMOD_LOWMEM | FMOD_LOOP_OFF);
	channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
	channel->set3DMinMaxDistance(distance, 10000.f);
	channel->setPaused(false);
	playing_sounds.push_back(channel);
	return channel;
}

//=================================================================================================
void SoundManager::StopSounds()
{
	if(disabled_sound)
		return;
	for(vector<FMOD::Channel*>::iterator it = playing_sounds.begin(), end = playing_sounds.end(); it != end; ++it)
		(*it)->stop();
	playing_sounds.clear();
}

//=================================================================================================
void SoundManager::SetListenerPosition(const Vec3& pos, const Vec3& dir, const Vec3& up)
{
	if(disabled_sound)
		return;
	system->set3DListenerAttributes(0, (const FMOD_VECTOR*)&pos, nullptr, (const FMOD_VECTOR*)&dir, (const FMOD_VECTOR*)&up);
}

//=================================================================================================
void SoundManager::SetSoundVolume(int volume)
{
	assert(InRange(volume, 0, 100));
	sound_volume = volume;
	play_sound = !nosound && volume > 0;
	if(!disabled_sound)
		group_default->setVolume(float(nosound ? 0 : sound_volume) / 100);
}

//=================================================================================================
void SoundManager::SetMusicVolume(int volume)
{
	assert(InRange(volume, 0, 100));
	music_volume = volume;
	if(!disabled_sound)
		group_music->setVolume(float(nomusic ? 0 : music_volume) / 100);
}

//=================================================================================================
bool SoundManager::UpdateChannelPosition(FMOD::Channel* channel, const Vec3& pos)
{
	assert(channel);

	bool is_playing;
	if(channel->isPlaying(&is_playing) == FMOD_OK && is_playing)
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
	bool is_playing;
	if(channel->isPlaying(&is_playing) == FMOD_OK)
		return is_playing;
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

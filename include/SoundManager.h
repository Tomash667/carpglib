#pragma once

//-----------------------------------------------------------------------------
#include "Sound.h"

//-----------------------------------------------------------------------------
class SoundManager
{
	friend class DefaultDeviceHandler;
public:
	SoundManager();
	~SoundManager();
	void Init();
	void Disable() { assert(!initialized); disabled = true; }
	void Update(float dt);
	int LoadSound(Sound* sound);
	void PlayMusic(Music* music, bool loop = true);
	void PlayMusic(MusicList* musicList, bool delayed = false);
	void PlaySound2d(Sound* sound);
	void PlaySound3d(Sound* sound, const Vec3& pos, float distance);
	FMOD::Channel* CreateChannel(Sound* sound, const Vec3& pos, float distance);
	void StopSounds();
	void StopMusic();
	bool UpdateChannelPosition(FMOD::Channel* channel, const Vec3& pos);

	bool IsPlaying(FMOD::Channel* channel);
	bool IsDisabled() const { return disabled; }
	bool IsMusicEnded() const { return musicEnded; }
	bool CanPlaySound() const { return playSound; }
	int GetSoundVolume() const { return soundVolume; }
	int GetMusicVolume() const { return musicVolume; }
	const Guid& GetDevice() const { return device; }
	void GetDevices(vector<pair<Guid, string>>& devices) const;
	void SetListenerPosition(const Vec3& pos, const Vec3& dir, const Vec3& up = Vec3::Up);
	void SetSoundVolume(int volume);
	void SetMusicVolume(int volume);
	void SetDevice(Guid device);

private:
	void HandleDefaultDeviceChange(const Guid& device);
	void SetupMusics();

	FMOD::System* system;
	FMOD::ChannelGroup* groupDefault, *groupMusic;
	FMOD::Channel* currentMusic;
	MusicList* musicList;
	MusicList tmpMusicList;
	Music* lastMusic;
	DefaultDeviceHandler* handler;
	vector<FMOD::Channel*> playingSounds;
	vector<FMOD::Channel*> fallbacks;
	vector<Music*> musics;
	vector<Buffer*> soundBufs;
	CriticalSection criticalSection;
	Guid device, defaultDevice, newDevice;
	int soundVolume, musicVolume;
	bool initialized, musicEnded, disabled, playSound;
};

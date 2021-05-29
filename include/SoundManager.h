#pragma once

//-----------------------------------------------------------------------------
class SoundManager
{
	friend class DefaultDeviceHandler;
public:
	SoundManager();
	~SoundManager();
	void Init();
	void Disable(bool nosound, bool nomusic);
	void Update(float dt);
	int LoadSound(Sound* sound);
	void PlayMusic(Music* music);
	void PlaySound2d(Sound* sound);
	void PlaySound3d(Sound* sound, const Vec3& pos, float distance);
	FMOD::Channel* CreateChannel(Sound* sound, const Vec3& pos, float distance);
	void StopSounds();
	void SetListenerPosition(const Vec3& pos, const Vec3& dir, const Vec3& up = Vec3(0, 1, 0));
	void SetSoundVolume(int volume);
	void SetMusicVolume(int volume);
	void SetMusicLoop(bool music_loop) { this->music_loop = music_loop; }
	void SetDevice(Guid device);
	bool UpdateChannelPosition(FMOD::Channel* channel, const Vec3& pos);

	bool IsPlaying(FMOD::Channel* channel);
	bool IsDisabled() const { return disabled_sound; }
	bool IsSoundDisabled() const { return nosound; }
	bool IsMusicDisabled() const { return nomusic; }
	bool IsMusicEnded() const { return music_ended; }
	bool IsMusicLoop() const { return music_loop; }
	bool CanPlaySound() const { return play_sound; }
	int GetSoundVolume() const { return sound_volume; }
	int GetMusicVolume() const { return music_volume; }
	const Guid& GetDevice() const { return device; }
	void GetDevices(vector<pair<Guid, string>>& devices) const;

private:
	void HandleDefaultDeviceChange(const Guid& device);

	FMOD::System* system;
	FMOD::ChannelGroup* group_default, *group_music;
	FMOD::Channel* current_music;
	DefaultDeviceHandler* handler;
	vector<FMOD::Channel*> playing_sounds;
	vector<FMOD::Channel*> fallbacks;
	vector<Buffer*> sound_bufs;
	CriticalSection criticalSection;
	Guid device, defaultDevice, newDevice;
	int sound_volume, music_volume; // 0-100
	bool initialized, music_ended, disabled_sound, play_sound, nosound, nomusic, music_loop;
};

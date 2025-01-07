#pragma once

#include "PCMFunctions.h"

namespace RS
{
	class PortAudio;
	class Sound;
	class ChannelGroup;
	class AudioSystem
	{
	public:
		AudioSystem();
		~AudioSystem();

		static AudioSystem* Get();

		void Init();
		void Destroy();

		void Update();

		void SetMasterVolume(float volume);
		void SetStreamVolume(float volume);
		void SetEffectsVolume(float volume);

		// Assume the sound file is in stereo (Two channels). (This can be fixed in the PCMFunctions.h when processing the data!)
		Sound* CreateSound(const std::string& filePath);
		void RemoveSound(Sound* pSound);

		/*
			Will stream the sound when playing and loop it when reaching the end.
			Assume the sound file is in stereo (Two channels). (This can be fixed in the PCMFunctions.h when processing the data!)
			TODO: Fix memory leaks when using this! (Have to do with the dr library for loading from file)
		*/
		Sound* CreateStream(const std::string& filePath);
		void RemoveStream(Sound* pSoundStream);

		static void LoadStreamFile(SoundHandle* pSoundHandle, const std::string& filePath);
		static void LoadEffectFile(SoundHandle* pSoundHandle, const std::string& filePath, PaSampleFormat format);

		// Debug
		void DrawAudioSettings();

	private:
		void DrawSoundSettings(Sound* sound, uint32_t index);

	private:
		std::vector<Sound*> m_Sounds;
		std::vector<Sound*> m_SoundStreams;
		PortAudio* m_pPortAudio = nullptr;

		float m_EffectsVolume	= 1.f;
		float m_StreamVolume	= 1.f;
		float m_MasterVolume	= 1.f;
	};
}
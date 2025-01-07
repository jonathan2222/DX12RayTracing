#pragma once

#include "Types.h"
#include "dr_mp3.h"
#include "dr_wav.h"

#define DEFAULT_SAMPLE_RATE 44100

namespace RS
{
	struct SoundHandle
	{
		enum Type { TYPE_NON, TYPE_MP3, TYPE_WAV };
		Type type{ TYPE_NON };

		bool asEffect{ true };
		uint64 pos{ 0 };
		uint32 sampleRate{ DEFAULT_SAMPLE_RATE };

		// Used for streams.
		drmp3 mp3; 
		drwav wav;

		// Used for effects.
		float* directDataF32{ nullptr };
		int16* directDataI16{ nullptr };
		uint32 nChannels{ 0 };
		uint64 totalFrameCount{ 0 };
	};
}
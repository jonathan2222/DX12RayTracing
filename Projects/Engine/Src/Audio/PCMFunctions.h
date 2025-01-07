#pragma once

#include "PortAudio.h"
#include "DR/dr_helper.h"
#include "Filters/Filter.h"
#include "SoundData.h"

#include <cstdint>
#include <algorithm>
#include <vector>
#include <glm/glm.hpp>

namespace RS
{
	class PCM
	{
	private:
		struct OutputData
		{
			float* pOutF32 = nullptr;
			int16* pOutI16 = nullptr;
		};

	public:

		struct UserData
		{
			SoundData soundData;

			SoundHandle handle;
			bool finished = false;
			PaSampleFormat sampleFormat = paFloat32;
			std::vector<Filter*> filters;
		};

		static void SetPos(SoundHandle* pHandle, uint64 newPos);

		static int PaCallbackPCM(const void* pInputBuffer, void* pOutputBuffer, unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* pTimeInfo, PaStreamCallbackFlags statusFlags, void* pUserData);

	private:
		static uint64 ReadPCMFrames(SoundHandle* pHandle, uint64 framesToRead, PaSampleFormat format, void* pOutBuffer);
		static uint64 ReadPCMFramesEffect(SoundHandle* pHandle, uint64 framesToRead, PaSampleFormat format, void* pOutBuffer);
		static std::pair<float, float> GetSamples(UserData* pData, OutputData* pOutputData);
		static std::pair<float, float> GetSamples(UserData* pData, OutputData* pOutputData, uint32 index);
		static OutputData GetOutputData(UserData* pData, void* pOut);
		static void ApplyToEar(UserData* pData, OutputData* pOutputData, float value);
		static void ApplyToEar(UserData* pData, OutputData* pOutputData, float value, uint32 index);
		static void ApplyToEar(UserData* pData, OutputData* pOutputData, std::pair<float, float> values);
		static void ApplyToEar(UserData* pData, OutputData* pOutputData, std::pair<float, float> values, uint32 index);
		static uint64 ReadPCM(UserData* pData, uint64 framesPerBuffer, void* pOutBuffer);
		static int Finish(UserData* pData, uint64 framesRead, uint64 framesPerBuffer, bool continueFlag);
	};
}
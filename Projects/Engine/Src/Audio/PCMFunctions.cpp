#include "PreCompiled.h"
#include "PCMFunctions.h"

#include <limits>
#include <glm/gtx/vector_angle.hpp>

void RS::PCM::SetPos(SoundHandle* pHandle, uint64 newPos)
{
	pHandle->pos = newPos;
	if (pHandle->asEffect == false)
	{
		switch (pHandle->type)
		{
		case SoundHandle::Type::TYPE_MP3:
			drmp3_seek_to_pcm_frame(&pHandle->mp3, newPos);
			break;
		case SoundHandle::Type::TYPE_WAV:
			drwav_seek_to_pcm_frame(&pHandle->wav, newPos);
			break;
		default:
			break;
		}
	}
}

int RS::PCM::PaCallbackPCM(const void* pInputBuffer, void* pOutputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* pTimeInfo, PaStreamCallbackFlags statusFlags, void* pUserData)
{
	UserData* pData = (UserData*)pUserData;
	uint64 framesRead = ReadPCM(pData, framesPerBuffer, pOutputBuffer);
	OutputData outputData = GetOutputData(pData, pOutputBuffer);

	for (Filter* pFt : pData->filters)
		pFt->Begin();
	bool shouldContinue = false;
	for (uint64_t t = 0; t < framesPerBuffer; t++)
	{
		auto samples = GetSamples(pData, &outputData);
		float leftEar = samples.first;
		float rightEar = samples.second;
		for (Filter* pFt : pData->filters)
		{
			auto res = pFt->Process(&pData->soundData, leftEar, rightEar);
			leftEar = std::get<0>(res);
			rightEar = std::get<1>(res);
			shouldContinue = shouldContinue && (abs(leftEar) > 0.00000001f ? true : shouldContinue);
			shouldContinue = shouldContinue && (abs(rightEar) > 0.00000001f ? true : shouldContinue);
		}

		float volume = pData->soundData.Volume * pData->soundData.GroupVolume * pData->soundData.MasterVolume;
		ApplyToEar(pData, &outputData, { leftEar * volume, rightEar * volume });
	}

	return Finish(pData, framesRead, framesPerBuffer, shouldContinue);
}

uint64 RS::PCM::ReadPCMFrames(SoundHandle* pHandle, uint64 framesToRead, PaSampleFormat format, void* pOutBuffer)
{
	drmp3_uint64 frameCount = (drmp3_uint64)framesToRead;
	uint64 framesRead = 0;
	switch (pHandle->type)
	{
	case SoundHandle::Type::TYPE_MP3:
		if (format == paFloat32) framesRead = drmp3_read_pcm_frames_f32(&pHandle->mp3, frameCount, (float*)pOutBuffer);
		if (format == paInt16) framesRead = drmp3_read_pcm_frames_s16(&pHandle->mp3, frameCount, (int16*)pOutBuffer);
		break;
	case SoundHandle::Type::TYPE_WAV:
		if (format == paFloat32) framesRead = drwav_read_pcm_frames_f32(&pHandle->wav, frameCount, (float*)pOutBuffer);
		if (format == paInt16) framesRead = drwav_read_pcm_frames_s16(&pHandle->wav, frameCount, (int16*)pOutBuffer);
		break;
	default:
		break;
	}
	pHandle->pos += frameCount;
	return framesRead;
}

uint64 RS::PCM::ReadPCMFramesEffect(SoundHandle* pHandle, uint64 framesToRead, PaSampleFormat format, void* pOutBuffer)
{
	uint64 framesRead = framesToRead;
	// Do not copy data if sound is already processed. 
	// - This is bad, I could have looped around but then the readPCMFrames function would not work (Because the dr library does not do this).
	if (pHandle->pos >= pHandle->totalFrameCount)
		framesRead = 0;
	else
	{
		// Copy frames to the outBuffer.
		framesRead = std::min(framesToRead, pHandle->totalFrameCount - pHandle->pos);
		if (format == paFloat32)
			memcpy(pOutBuffer, &pHandle->directDataF32[pHandle->pos * 2], framesRead * 2 * sizeof(float));
		else if (format == paInt16)
			memcpy(pOutBuffer, &pHandle->directDataI16[pHandle->pos * 2], framesRead * 2 * sizeof(int16_t));
		pHandle->pos += framesRead;
	}
	return framesRead;
}

std::pair<float, float> RS::PCM::GetSamples(UserData* pData, OutputData* pOutputData)
{
	float left = pData->sampleFormat == paInt16 ? (float)(*(pOutputData->pOutI16)) : *(pOutputData->pOutF32);
	float right = pData->sampleFormat == paInt16 ? (float)(*(pOutputData->pOutI16 + 1)) : *(pOutputData->pOutF32 + 1);
	return std::pair<float, float>(left, right);
}

std::pair<float, float> RS::PCM::GetSamples(UserData* pData, OutputData* pOutputData, uint32 index)
{
	float left = pData->sampleFormat == paInt16 ? (float)(pOutputData->pOutI16[index]) : pOutputData->pOutF32[index];
	float right = pData->sampleFormat == paInt16 ? (float)(pOutputData->pOutI16[index + 1]) : pOutputData->pOutF32[index + 1];
	return std::pair<float, float>(left, right);
}

RS::PCM::OutputData RS::PCM::GetOutputData(UserData* pData, void* pOut)
{
	OutputData outData;
	outData.pOutF32 = (float*)pOut;
	outData.pOutI16 = (int16*)pOut;
	return outData;
}

void RS::PCM::ApplyToEar(UserData* pData, OutputData* pOutputData, float value)
{
	if (pData->sampleFormat == paInt16)
		*pOutputData->pOutI16++ = (int16)std::clamp(value, (float)INT16_MIN, (float)INT16_MAX);
	else if (pData->sampleFormat == paFloat32)
		*pOutputData->pOutF32++ = value;
}

void RS::PCM::ApplyToEar(UserData* pData, OutputData* pOutputData, float value, uint32 index)
{
	if (pData->sampleFormat == paInt16)
		pOutputData->pOutI16[index] = (int16)std::clamp(value, (float)INT16_MIN, (float)INT16_MAX);
	else if (pData->sampleFormat == paFloat32)
		pOutputData->pOutF32[index] = value;
}

void RS::PCM::ApplyToEar(UserData* pData, OutputData* pOutputData, std::pair<float, float> values)
{
	ApplyToEar(pData, pOutputData, values.first);
	ApplyToEar(pData, pOutputData, values.second);
}

void RS::PCM::ApplyToEar(UserData* pData, OutputData* pOutputData, std::pair<float, float> values, uint32 index)
{
	ApplyToEar(pData, pOutputData, values.first, index);
	ApplyToEar(pData, pOutputData, values.second, index + 1);
}

uint64 RS::PCM::ReadPCM(UserData* pData, uint64 framesPerBuffer, void* pOutBuffer)
{
	uint64 framesRead = 0;
	if (pData->handle.asEffect)
		framesRead = ReadPCMFramesEffect(&pData->handle, framesPerBuffer, pData->sampleFormat, pOutBuffer);
	else
		framesRead = ReadPCMFrames(&pData->handle, framesPerBuffer, pData->sampleFormat, pOutBuffer);
	return framesRead;
}

int RS::PCM::Finish(UserData* pData, uint64 framesRead, uint64 framesPerBuffer, bool continueFlag)
{
	if (pData->soundData.Loop)
	{
		// Reset and play again.
		if (framesRead < framesPerBuffer && !continueFlag)
			SetPos(&pData->handle, 0);
		return paContinue;
	}
	else
	{
		// Reset and stop the stream.
		if (framesRead < framesPerBuffer && !continueFlag)
		{
			SetPos(&pData->handle, 0);
			pData->finished = true;
		}
		return pData->finished ? paComplete : paContinue;
	}
}

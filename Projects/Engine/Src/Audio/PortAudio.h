#pragma once

#include <PortAudio/portaudio.h>

#define PORT_AUDIO_CHECK(res, ...) \
	{ \
		PaError r = res; \
		if(r != paNoError) { \
			RS_LOG_ERROR("PortAudio error! ({0}) {1}", r, Pa_GetErrorText(res)); \
			RS_ASSERT(false, __VA_ARGS__); \
		} \
	}

namespace RS
{
	class PortAudio
	{
	public:
		PortAudio();
		~PortAudio();

		void Init();
		void Destroy();

		PaDeviceIndex GetDeviceIndex();
		const PaDeviceInfo* GetDeviceInfo();

	private:
		PaDeviceIndex m_DeviceIndex = 0;
		mutable const PaDeviceInfo* m_pDeviceInfo = nullptr;
		PaError m_ErrorCode = paNoError;
	};
}
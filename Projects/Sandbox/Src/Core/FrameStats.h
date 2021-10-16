#pragma once

namespace RS
{
	struct FrameStats
	{
		struct Frame
		{
			float currentDT = 0.0f;
			float avgDTMs = 0.0f;
			float avgFPS = 0.0f;
			float minDT = 3.402823466e+38F;
			float maxDT = 1.175494351e-38F;
		} frame;

		struct FixedUpdate
		{
			float updateCallsRatio = 0.f;
			float fixedFPS = 60.f;
			uint32 maxUpdateCalls = 3;
		} fixedUpdate;
	};
}
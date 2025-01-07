#pragma once

#include <glm/vec3.hpp>

namespace RS
{
	struct SoundData
	{
		float MasterVolume	= 1.f;	// Volume for all sounds
		float GroupVolume	= 1.f;	// Volume for effects/streams
		float Volume		= 1.f;	// Volume for this sound

		bool Loop			= false;
		glm::vec3 sourcePos;
		glm::vec3 receiverPos;
		glm::vec3 receiverLeft;
		glm::vec3 receiverUp;
	};
}
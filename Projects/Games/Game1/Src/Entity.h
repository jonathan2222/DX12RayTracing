#pragma once

#include "Maths/GLMDefines.h"
#include <glm/glm.hpp>

class Entity
{
public:
	Entity(const glm::vec2& pos, const glm::vec2& vel)
		: m_Position(pos), m_Velocity(vel) {}

	glm::vec2 m_Position;
	glm::vec2 m_Velocity;

private:

};
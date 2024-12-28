#pragma once

#include "Maths/GLMDefines.h"
#include <glm/glm.hpp>

class Entity
{
public:
	enum class Type : uint
	{
		EASY_ENEMY = 0,
		NORMAL_ENEMY,
		COUNT
	};

public:
	Entity(Type type, const glm::vec2& pos, const glm::vec2& vel)
		: m_Type(type), m_Position(pos), m_Velocity(vel) {}

	Type m_Type;
	glm::vec2 m_Position;
	glm::vec2 m_Velocity;

private:

};
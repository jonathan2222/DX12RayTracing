#pragma once

#include "Maths/GLMDefines.h"
#include <glm/glm.hpp>
#include "Types.h"

class Entity
{
public:
	enum class Type : uint
	{
		EASY_ENEMY = 0,
		NORMAL_ENEMY,
		COUNT
	};

	struct EnemyInfo
	{
		glm::vec2 size;
		glm::vec3 color;
	};

	static EnemyInfo GetEnemyInfoFromType(Type type);

public:
	Entity(Type type, const glm::vec2& pos, const glm::vec2& vel, float health = 100.f)
		: m_Type(type), m_Position(pos), m_Velocity(vel), m_Health(health) {}

	Type m_Type;
	glm::vec2 m_Position;
	glm::vec2 m_Velocity;
	float m_Health;

};
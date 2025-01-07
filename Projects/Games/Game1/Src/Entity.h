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
		ENEMY_COUNT,
		BIT = ENEMY_COUNT,
		COUNT
	};

	struct EntityInfo
	{
		glm::vec2 size;
		glm::vec3 color;
		uint attack = 0;
		uint bitCount = 0;
	};

	static EntityInfo GetEntityInfoFromType(Type type);

	static bool IsEnemy(Type type);

public:
	Entity(Type type, const glm::vec2& pos, const glm::vec2& vel, float friction = 0.f, float health = 100.f)
		: m_Type(type), m_Position(pos), m_Velocity(vel), m_Friction(friction), m_Health(health), m_Scale(1.f) {}

	Type m_Type;
	glm::vec2 m_Position;
	glm::vec2 m_Velocity;
	float m_Friction;
	float m_Health;
	float m_Scale;
};
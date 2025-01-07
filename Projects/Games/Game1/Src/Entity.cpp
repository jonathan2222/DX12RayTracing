#include "Entity.h"

Entity::EntityInfo Entity::GetEntityInfoFromType(Entity::Type type)
{
	EntityInfo info;
	switch (type)
	{
	case Entity::Type::EASY_ENEMY:
		info.size = glm::vec2(2.f, 2.f);
		info.color = glm::vec3(0.5f, 0.0f, 0.0f);
		info.attack = 10;
		info.bitCount = 5;
		return info;
	case Entity::Type::NORMAL_ENEMY:
		info.size = glm::vec2(2.f, 2.f);
		info.color = glm::vec3(0.5f, 0.0f, 0.0f);
		info.attack = 20;
		info.bitCount = 10;
		return info;
	case Entity::Type::BIT:
	default:
		info.size = glm::vec2(0.25f, 0.25f);
		info.color = glm::vec3(0.5f, 0.1f, 0.1f);
		info.attack = 0;
		return info;
	}
}

bool Entity::IsEnemy(Type type)
{
	return (uint)type < (uint)Type::ENEMY_COUNT;
}

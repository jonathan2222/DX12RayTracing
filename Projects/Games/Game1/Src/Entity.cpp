#include "Entity.h"

Entity::EnemyInfo Entity::GetEnemyInfoFromType(Entity::Type type)
{
	EnemyInfo info;
	switch (type)
	{
	case Entity::Type::EASY_ENEMY:
		info.size = glm::vec2(2.f, 2.f);
		info.color = glm::vec3(1.f, 0.f, 0.f);
		return info;
	case Entity::Type::NORMAL_ENEMY:
	default:
		info.size = glm::vec2(2.f, 2.f);
		info.color = glm::vec3(1.f, 0.f, 0.f);
		return info;
	}
}

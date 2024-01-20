#pragma once

#include "Maths/RSVector.h"

namespace RS
{
	struct Vertex
	{
		Vec3 position;
		Vec3 normal;
		Vec2 uv;
	};

	class Mesh
	{
	public:
		std::vector<Vertex> vertices;
		std::vector<uint32> indices;
	};

	class FBXLoader
	{
	public:
		static Mesh* Load(const std::string& path);
	};
}
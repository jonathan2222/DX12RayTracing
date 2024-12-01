#pragma once

#include "Maths/GLMDefines.h"
#include <glm/glm.hpp>

#include <vector>

namespace RS
{
	class Camera2D
	{
	public:
		struct Plane
		{
			alignas(16) glm::vec3 normal;
			alignas(16) glm::vec3 point;
		};
	public:

		void Init(float left, float right, float bottom, float top, float nearPlane, float farPlane, const glm::vec3& position);
		void Destroy();

		void Update(float dt);

		void SetPosition(const glm::vec3& position);

		glm::mat4 GetMatrix() const;
		glm::mat4 GetProjection() const;
		glm::mat4 GetView() const;
		const glm::vec3& GetPosition() const;
		const glm::vec3& GetRight() const;
		const glm::vec3& GetUp() const;

		const std::vector<Plane>& GetPlanes() const;

	private:
		enum Side { NEAR_P = 0, FAR_P, LEFT_P, RIGHT_P, TOP_P, BOTTOM_P };

	private:
		void UpdatePlanes();

	private:
		std::vector<Plane> m_Planes;

		float m_NearPlane;
		float m_FarPlane;
		float m_Roll;
		float m_Left;
		float m_Right;
		float m_Bottom;
		float m_Top;

		glm::vec3 m_GlobalUp;

		glm::vec3 m_UpV;
		glm::vec3 m_ForwardV;
		glm::vec3 m_RightV;

		glm::vec3 m_Position;
	};
}
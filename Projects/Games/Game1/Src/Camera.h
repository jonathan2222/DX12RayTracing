#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace RS
{
	class Camera
	{
	public:
		struct Plane
		{
			alignas(16) glm::vec3 normal;
			alignas(16) glm::vec3 point;
		};
	public:

		void Init(float aspect, float fov, const glm::vec3& position, const glm::vec3& target, float speed, float hasteSpeed);
		void Destroy();

		void Update(float dt);

		void SetPosition(const glm::vec3& position);
		void SetSpeed(float speed);

		glm::mat4 GetMatrix() const;
		glm::mat4 GetProjection() const;
		glm::mat4 GetView() const;
		const glm::vec3& GetPosition() const;
		glm::vec3 GetDirection() const;
		const glm::vec3& GetRight() const;
		const glm::vec3& GetUp() const;

		const std::vector<Plane>& GetPlanes() const;

	private:
		enum Side { NEAR_P = 0, FAR_P, LEFT_P, RIGHT_P, TOP_P, BOTTOM_P };

	private:
		void UpdatePlanes();

	private:
		std::vector<Camera::Plane> m_Planes;

		float m_Speed;
		float m_SpeedFactor;
		float m_HasteSpeed;
		float m_Fov;
		float m_NearPlane;
		float m_FarPlane;
		float m_Yaw;
		float m_Pitch;
		float m_Roll;
		float m_Aspect;
		float m_NearHeight;
		float m_NearWidth;
		float m_FarHeight;
		float m_FarWidth;

		glm::vec3 m_GlobalUp;

		glm::vec3 m_Up;
		glm::vec3 m_Forward;
		glm::vec3 m_Right;

		glm::vec3 m_Position;
		glm::vec3 m_Target;

		bool m_GravityOn;
		float m_PlayerHeight;
	};
}
#include "Camera.h"

#include <RSEngine.h>

#include "Maths/GLMDefines.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/norm.hpp"

#define FRUSTUM_SHRINK_FACTOR -5.f

void RS::Camera::Init(float aspect, float fov, const glm::vec3& position, const glm::vec3& target, float speed, float hasteSpeed)
{
	m_PlayerHeight = 2.0f;
	m_Position = position;
	m_Aspect = aspect;
	m_Target = target;
	m_Speed = speed;
	m_HasteSpeed = hasteSpeed;
	m_GlobalUp = glm::vec3(0.f, 1.f, 0.f);

	m_Forward = glm::normalize(m_Target - m_Position);
	m_Up = m_GlobalUp;
	m_Right = glm::cross(m_Forward, m_Up);

	m_Fov = fov;
	m_NearPlane = 0.1f;
	m_FarPlane = 1000.f;
	m_Yaw = std::atan2(m_Forward.z, m_Forward.x) * 180.f / 3.1415f;
	m_Pitch = std::asin(-m_Forward.y) * 180.f / 3.1415f;
	m_Roll = 0;
	m_SpeedFactor = 1.f;

	float tang = std::tan(m_Fov / 2);
	m_NearHeight = m_NearPlane * tang * 2.0f;
	m_NearWidth = m_NearHeight * m_Aspect;
	m_FarHeight = m_FarPlane * tang * 2.0f;
	m_FarWidth = m_FarHeight * m_Aspect;

	m_Planes.resize(6);
	UpdatePlanes();
}

void RS::Camera::Destroy()
{
}

void RS::Camera::Update(float dt)
{
	auto pInput = RS::Input::Get();

	if (pInput->IsKeyPressed(RS::Key::A))
		m_Position -= m_Right * m_Speed * dt * m_SpeedFactor;
	if (pInput->IsKeyPressed(RS::Key::D))
		m_Position += m_Right * m_Speed * dt * m_SpeedFactor;

	if (pInput->IsKeyPressed(RS::Key::W))
		m_Position += m_Forward * m_Speed * dt * m_SpeedFactor;
	if (pInput->IsKeyPressed(RS::Key::S))
		m_Position -= m_Forward * m_Speed * dt * m_SpeedFactor;

	if (pInput->IsKeyPressed(RS::Key::E))
		m_Position += m_GlobalUp * m_Speed * dt * m_SpeedFactor;
	if (pInput->IsKeyPressed(RS::Key::Q))
		m_Position -= m_GlobalUp * m_Speed * dt * m_SpeedFactor;

	/*if (input.getKeyState(GLFW_KEY_F) == Input::KeyState::FIRST_RELEASED)
	{
		this->gravityOn ^= true;
		JAS_INFO("Toggled gravity: {}", this->gravityOn ? "On" : "Off");
	}

	if (this->gravityOn)
	{
		this->position.y = floor + this->playerHeight;
	}*/

	if (pInput->IsKeyPressed(RS::Key::LEFT_SHIFT))
		m_SpeedFactor = m_HasteSpeed;
	else
		m_SpeedFactor = 1.f;

	glm::vec2 cursorDelta = pInput->GetCursorDelta();
	cursorDelta *= 0.1f;

	if (glm::length2(cursorDelta) > glm::epsilon<float>())
	{
		m_Yaw += cursorDelta.x;
		m_Pitch -= cursorDelta.y;

		//RS_LOG_WARN("yaw: {}, pitch: {}", this->yaw, this->pitch);

		if (m_Pitch > 89.0f)
			m_Pitch = 89.0f;
		if (m_Pitch < -89.0f)
			m_Pitch = -89.0f;

		glm::vec3 direction;
		direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		direction.y = sin(glm::radians(m_Pitch));
		direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

		m_Forward = glm::normalize(direction);
		m_Right = glm::normalize(glm::cross(m_Forward, m_GlobalUp));
		m_Up = glm::cross(m_Right, m_Forward);

	}
	UpdatePlanes();
	m_Target = m_Position + m_Forward;
}

void RS::Camera::SetPosition(const glm::vec3& position)
{
	m_Position = position;
}

void RS::Camera::SetSpeed(float speed)
{
	m_Speed = speed;
}

glm::mat4 RS::Camera::GetMatrix() const
{
	return GetProjection() * GetView();
}

glm::mat4 RS::Camera::GetProjection() const
{
	glm::mat4 proj = glm::perspectiveRH(m_Fov, m_Aspect, m_NearPlane, m_FarPlane);
	//proj[1][1] *= -1;
	return proj;
}

glm::mat4 RS::Camera::GetView() const
{
	return glm::lookAtRH(m_Position, m_Target, m_Up);
}

const glm::vec3& RS::Camera::GetPosition() const
{
    return m_Position;
}

glm::vec3 RS::Camera::GetDirection() const
{
	return glm::normalize(m_Target - m_Position);
}

const glm::vec3& RS::Camera::GetRight() const
{
    return m_Right;
}

const glm::vec3& RS::Camera::GetUp() const
{
    return m_Up;
}

const std::vector<RS::Camera::Plane>& RS::Camera::GetPlanes() const
{
	return m_Planes;
}

void RS::Camera::UpdatePlanes()
{
	glm::vec3 point;
	glm::vec3 nearCenter = m_Position + m_Forward * m_NearPlane;
	glm::vec3 farCenter = m_Position + m_Forward * m_FarPlane;

	m_Planes[NEAR_P].normal = m_Forward;
	m_Planes[NEAR_P].point = nearCenter;

	m_Planes[FAR_P].normal = -m_Forward;
	m_Planes[FAR_P].point = farCenter;

	point = nearCenter + m_Up * (m_NearHeight / 2) - m_Right * (m_NearWidth / 2);
	m_Planes[LEFT_P].normal = glm::normalize(glm::cross(point - m_Position, m_Up));
	m_Planes[LEFT_P].point = point + m_Planes[LEFT_P].normal * FRUSTUM_SHRINK_FACTOR;

	m_Planes[TOP_P].normal = glm::normalize(glm::cross(point - m_Position, m_Right));
	m_Planes[TOP_P].point = point;

	point = nearCenter - m_Up * (m_NearHeight / 2) + m_Right * (m_NearWidth / 2);
	m_Planes[RIGHT_P].normal = glm::normalize(glm::cross(point - m_Position, -m_Up));
	m_Planes[RIGHT_P].point = point + m_Planes[RIGHT_P].normal * FRUSTUM_SHRINK_FACTOR;

	m_Planes[BOTTOM_P].normal = glm::normalize(glm::cross(point - m_Position, -m_Right));
	m_Planes[BOTTOM_P].point = point;
}

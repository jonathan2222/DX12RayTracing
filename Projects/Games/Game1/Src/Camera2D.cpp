#include "Camera2D.h"

#include <RSEngine.h>

#include "Maths/GLMDefines.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/norm.hpp"

#define FRUSTUM_SHRINK_FACTOR -5.f

#include "Core/Console.h"
#include "Core/VMap.h"
RS_ADD_GLOBAL_CONSOLE_VAR(float, "Game1App.Camera2D.Left", g_Left, -10.f, "");
RS_ADD_GLOBAL_CONSOLE_VAR(float, "Game1App.Camera2D.Right", g_Right, 10.f, "");
RS_ADD_GLOBAL_CONSOLE_VAR(float, "Game1App.Camera2D.Bottom", g_Bottom, -10.f, "");
RS_ADD_GLOBAL_CONSOLE_VAR(float, "Game1App.Camera2D.Top", g_Top, 10.f, "");
RS_ADD_GLOBAL_CONSOLE_VAR(float, "Game1App.Camera2D.Near", g_Near, FLT_EPSILON, "");
RS_ADD_GLOBAL_CONSOLE_VAR(float, "Game1App.Camera2D.Far", g_Far, 1000.f, "");

namespace Camera2D_Local
{
	RS::VMap g_DiskMap;
	const uint g_DiskVersion = 1;
	const std::string g_CameraSettingsPath = "Debug/GlobalCamera.cfg";
	void LoadFromDisk()
	{
		const std::string path = RS::Engine::GetTempFilePath() + g_CameraSettingsPath;
		RS::VMap map = RS::VMap::ReadFromDisk(path);

		if (!map.HasKey("version") || !map.IsOfType<uint>("version")) return;
		if (g_DiskVersion != (uint)map["version"]) return;

		g_DiskMap = std::move(map);

		g_Left = g_DiskMap["left"];
		g_Right = g_DiskMap["right"];
		g_Bottom = g_DiskMap["bottom"];
		g_Top = g_DiskMap["top"];
		g_Near = g_DiskMap["near"];
		g_Far = g_DiskMap["far"];
	}

	void SaveToDisk()
	{
		g_DiskMap["version"] = g_DiskVersion;
		g_DiskMap["left"] = g_Left;
		g_DiskMap["right"] = g_Right;
		g_DiskMap["bottom"] = g_Bottom;
		g_DiskMap["top"] = g_Top;
		g_DiskMap["near"] = g_Near;
		g_DiskMap["far"] = g_Far;
		const std::string path = RS::Engine::GetTempFilePath() + g_CameraSettingsPath;
		RS::VMap::WriteToDisk(g_DiskMap, path);
	}

	void Update()
	{
		if (g_DiskMap.IsEmpty())
		{
			g_DiskMap["version"] = g_DiskVersion;
			g_DiskMap["left"] = g_Left;
			g_DiskMap["right"] = g_Right;
			g_DiskMap["bottom"] = g_Bottom;
			g_DiskMap["top"] = g_Top;
			g_DiskMap["near"] = g_Near;
			g_DiskMap["far"] = g_Far;
			SaveToDisk();
		}

		if (!RS::Utils::AreValuesClose((float)g_DiskMap["left"], g_Left) ||
			!RS::Utils::AreValuesClose((float)g_DiskMap["right"], g_Right) ||
			!RS::Utils::AreValuesClose((float)g_DiskMap["bottom"], g_Bottom) ||
			!RS::Utils::AreValuesClose((float)g_DiskMap["top"], g_Top) ||
			!RS::Utils::AreValuesClose((float)g_DiskMap["near"], g_Near) ||
			!RS::Utils::AreValuesClose((float)g_DiskMap["far"], g_Far))
		{
			SaveToDisk();
		}
	}
}

void RS::Camera2D::Init(float left, float right, float bottom, float top, float nearPlane, float farPlane, const glm::vec3& position)
{
	m_Position = position;
	m_Left = left;
	m_Right = right;
	m_Bottom = bottom;
	m_Top = top;
	m_GlobalUp = glm::vec3(0.f, 1.f, 0.f);

	m_ForwardV = {0.f, 0.f, 1.0f};
	m_UpV = m_GlobalUp;
	m_RightV = glm::cross(m_ForwardV, m_UpV);

	m_NearPlane = nearPlane;
	m_FarPlane = farPlane;
	m_Roll = 0;

	//Camera2D_Local::LoadFromDisk();
	g_Left = m_Left;
	g_Right = m_Right;
	g_Bottom = m_Bottom;
	g_Top = m_Top;
	g_Near = m_NearPlane;
	g_Far = m_FarPlane;

	m_Planes.resize(6);
	UpdatePlanes();
}

void RS::Camera2D::Destroy()
{
}

void RS::Camera2D::Update(float dt)
{
	m_Left = g_Left;
	m_Right = g_Right;
	m_Bottom = g_Bottom;
	m_Top = g_Top;
	m_NearPlane = g_Near;
	m_FarPlane = g_Far;
	Camera2D_Local::Update();

	//auto pInput = RS::Input::Get();
	//
	//if (pInput->IsKeyPressed(RS::Key::A))
	//	m_Position -= m_RightV * m_Speed * dt * m_SpeedFactor;
	//if (pInput->IsKeyPressed(RS::Key::D))
	//	m_Position += m_RightV * m_Speed * dt * m_SpeedFactor;
	//
	//if (pInput->IsKeyPressed(RS::Key::W))
	//	m_Position += m_ForwardV * m_Speed * dt * m_SpeedFactor;
	//if (pInput->IsKeyPressed(RS::Key::S))
	//	m_Position -= m_ForwardV * m_Speed * dt * m_SpeedFactor;
	//
	//if (pInput->IsKeyPressed(RS::Key::E))
	//	m_Position += m_GlobalUp * m_Speed * dt * m_SpeedFactor;
	//if (pInput->IsKeyPressed(RS::Key::Q))
	//	m_Position -= m_GlobalUp * m_Speed * dt * m_SpeedFactor;

	UpdatePlanes();
}

void RS::Camera2D::SetPosition(const glm::vec3& position)
{
	m_Position = position;
}

glm::mat4 RS::Camera2D::GetMatrix() const
{
	return GetProjection() * GetView();
}

glm::mat4 RS::Camera2D::GetProjection() const
{
	glm::mat4 proj = glm::orthoRH(m_Left, m_Right, m_Bottom, m_Top, m_NearPlane, m_FarPlane);
	return proj;
}

glm::mat4 RS::Camera2D::GetView() const
{
	return glm::translate(-m_Position) * glm::rotate(-m_Roll, glm::vec3{0.f, 0.f, 1.0f});
}

const glm::vec3& RS::Camera2D::GetPosition() const
{
    return m_Position;
}

const glm::vec3& RS::Camera2D::GetRight() const
{
    return m_RightV;
}

const glm::vec3& RS::Camera2D::GetUp() const
{
    return m_UpV;
}

const std::vector<RS::Camera2D::Plane>& RS::Camera2D::GetPlanes() const
{
	return m_Planes;
}

void RS::Camera2D::UpdatePlanes()
{
	glm::vec3 point;
	glm::vec3 nearCenter = m_Position + m_ForwardV * m_NearPlane;
	glm::vec3 farCenter = m_Position + m_ForwardV * m_FarPlane;
	glm::vec3 right = m_RightV * m_Right;
	glm::vec3 left = m_RightV * m_Left;
	glm::vec3 top = m_UpV * m_Top;
	glm::vec3 bottom = m_UpV * m_Bottom;

	m_Planes[NEAR_P].normal = m_ForwardV;
	m_Planes[NEAR_P].point = nearCenter;

	m_Planes[FAR_P].normal = -m_ForwardV;
	m_Planes[FAR_P].point = farCenter;

	point = nearCenter + left + top;
	m_Planes[LEFT_P].normal = m_RightV;
	m_Planes[LEFT_P].point = point + m_Planes[LEFT_P].normal * FRUSTUM_SHRINK_FACTOR;

	m_Planes[TOP_P].normal = -m_UpV;
	m_Planes[TOP_P].point = point;

	point = nearCenter + right + bottom;
	m_Planes[RIGHT_P].normal = -m_RightV;
	m_Planes[RIGHT_P].point = point + m_Planes[RIGHT_P].normal * FRUSTUM_SHRINK_FACTOR;

	m_Planes[BOTTOM_P].normal = m_UpV;
	m_Planes[BOTTOM_P].point = point;
}

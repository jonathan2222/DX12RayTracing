#pragma once

#include <imgui.h>
#include <GLFW/glfw3.h>

struct GLFWwindow;
namespace RS
{
	class ImGuiAdapter
	{
	public:
		enum class ClientAPI
		{
			UNKNOWN,
			OPEN_GL,
			DX12,
			VULKAN
		};

		static bool Init(GLFWwindow* window, bool install_callbacks, ClientAPI clientAPI);
		static void Release();

		static void NewFrame();

		static void DisableInput();
		static void EnableInput();

	private:
		static void UpdateMousePosAndButtons();
		static void UpdateMouseCursor();
		static void UpdateGamepads();

		static const char* GetClipboardText(void* user_data);
		static void SetClipboardText(void* user_data, const char* text);
		static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
		static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void CharCallback(GLFWwindow* window, unsigned int c);
	private:
		inline static GLFWwindow* s_Window = nullptr;    // Main window
		inline static ClientAPI				s_ClientAPI = ClientAPI::UNKNOWN;
		inline static double				s_Time = 0.0;
		inline static bool					s_MouseJustPressed[ImGuiMouseButton_COUNT] = {};
		inline static GLFWcursor* s_MouseCursors[ImGuiMouseCursor_COUNT] = {};
		inline static bool					s_InstalledCallbacks = false;
		inline static bool					s_IsInputActive = true;

		// Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
		inline static GLFWmousebuttonfun	s_PrevUserCallbackMousebutton = nullptr;
		inline static GLFWscrollfun			s_PrevUserCallbackScroll = nullptr;
		inline static GLFWkeyfun			s_PrevUserCallbackKey = nullptr;
		inline static GLFWcharfun			s_PrevUserCallbackChar = nullptr;
	};
}
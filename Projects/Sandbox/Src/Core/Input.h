#pragma once
#pragma once

#include "Core/Key.h"
#include "Utils/Maths.h"

struct GLFWwindow;
namespace RS
{
	class Input
	{
	public:
		enum class ActiveState
		{
			RISING_EDGE,
			FALLING_EDGE
		};

		// This uses the same bits as GLFW uses.
		typedef uint32 ModFlags;
		enum ModFlag : ModFlags
		{
			NONE = 0,
			SHIFT = 1,
			CONTROL = 2,
			ALT = 4,
			SUPER = 8,
			CAPS_LOCK = 16,
			NUM_LOCK = 32,
			IGNORED = UINT32_MAX
		};

	public:
		Input() = default;
		~Input() = default;

		static std::shared_ptr<Input> Get();

		void Init();
		void PreUpdate();
		void PostUpdate(float dt);

		/*
		* Returns true if the key was pressed (held down) otherwise false.
		*/
		bool IsKeyPressed(const Key& key, ModFlags mods = ModFlag::IGNORED) const;

		/*
		* Returns true if the key was first held down or first released depending on the activeState.
		* However, it does not take into account the time it was held down!
		* Arguments:
		* - key:			The key in question.
		* - activeState:	The state of which the function should return when first pressed (RISING_EDGE) or first released (FALLING_EDGE).
		* Return:
		* - If the key was clicked or not.
		*/
		bool IsKeyClicked(const Key& key, ActiveState activeState = ActiveState::FALLING_EDGE, ModFlags mods = ModFlag::IGNORED) const;

		/*
		* Determains if the key that was pressed, should be counted as being held down or clicked.
		* Arguments:
		* - key:			The key in question.
		* - onClicked:		Callback function for when it is clicked (Will execute once).
		* - onHeld:			Callback function for when it is held down (Will execute once).
		* - timePresiod:	The time that describes if the key was clicked or pressed. If the key was held down shorter than this value, it will be counted as a clicked otherwise held down.
		* Return:
		* - If the key was being pressed or not.
		*/
		bool WasKeyClickedOrHeld(const Key& key, std::function<void(void)> onClicked, std::function<void(void)> onHeld, float timePeriod = 0.35f, ModFlags mods = ModFlag::IGNORED);

		/*
		* Returns the internal state of the key.
		* States are either RELEASED, FIRST_RELEASED, FIRST_PRESSED or PRESSED.
		*/
		KeyState GetKeyState(const Key& key) const;

		/*
		* Returns the time since it first was pressed.
		* If the key is released, the time will be 0.0f!
		*/
		float GetKeyTime(const Key& key) const;

		/*
		* Returns the mods active on this key.
		*/
		ModFlags GetKeyMods(const Key& key) const;

		glm::vec2 GetCursorDelta();
		glm::vec2 GetScrollDelta();
		glm::vec2 GetMousePos() const;

		/*
		* Returns true if the mouse button was pressed (held down) otherwise false.
		*/
		bool IsMBPressed(const MB& button, ModFlags mods = ModFlag::IGNORED) const;

		/*
		* Returns true if the mouse button was first held down or first released depending on the activeState.
		* However, it does not take into account the time it was held down!
		* Arguments:
		* - button:			The mouse button in question.
		* - activeState:	The state of which the function should return when first pressed (RISING_EDGE) or first released (FALLING_EDGE).
		* Return:
		* - If the mouse button was clicked or not.
		*/
		bool IsMBClicked(const MB& button, ActiveState activeState = ActiveState::FALLING_EDGE, ModFlags mods = ModFlag::IGNORED) const;

		/*
		* Determains if the mouse button that was pressed, should be counted as being held down or clicked.
		* Arguments:
		* - button:			The mouse button in question.
		* - onClicked:		Callback function for when it is clicked (Will execute once).
		* - onHeld:			Callback function for when it is held down (Will execute once).
		* - timePresiod:	The time that describes if the mouse button was clicked or pressed. If the mouse button was held down shorter than this value, it will be counted as a clicked otherwise held down.
		* Return:
		* - If the mouse button was being pressed or not.
		*/
		bool WasMBClickedOrHeld(const MB& button, std::function<void(void)> onClicked, std::function<void(void)> onHeld, float timePeriod = 0.35f, ModFlags mods = ModFlag::IGNORED);

		/*
		* Returns the internal state of the mouse button.
		* States are either RELEASED, FIRST_RELEASED, FIRST_PRESSED or PRESSED.
		*/
		KeyState GetMBState(const MB& button) const;

		/*
		* Returns the time since it first was pressed.
		* If the mouse button is released, the time will be 0.0f!
		*/
		float GetMBTime(const MB& button) const;

		/*
		* Returns the mods active on this mouse button.
		*/
		ModFlags GetMBMods(const MB& button) const;

		void CenterMouse() const;
		void LockMouse() const;
		void UnlockMouse() const;

		static std::string KeyStateToStr(KeyState state);

	private:
		struct KeyInfo
		{
			KeyState	State = KeyState::RELEASED;
			float		TimeHeld = 0.f;
			ModFlags	Mods = ModFlag::NONE;
		};

	private:
		bool		InternalIsKeyPressed(const KeyInfo& keyInfo, ModFlags mods) const;
		bool		InternalIsKeyClicked(const KeyInfo& keyInfo, ActiveState activeState, ModFlags mods) const;
		bool		InternalWasKeyClickedOrHeld(const KeyInfo& keyInfo, std::function<void(void)> onClicked, std::function<void(void)> onHeld, float timePeriod, ModFlags mods);
		KeyInfo& GetKeyInfo(const Key& key) const;
		KeyInfo& GetKeyInfo(const MB& button) const;

		void UpdateKeyInfo(KeyInfo& keyInfo, float dt);

		static void KeyCallback(GLFWwindow* wnd, int key, int scancode, int action, int mods);
		static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
		static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void MouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

	private:
		inline static std::unordered_map<Key, KeyInfo> s_KeyMap = std::unordered_map<Key, KeyInfo>();
		inline static std::unordered_map<MB, KeyInfo> s_MBMap = std::unordered_map<MB, KeyInfo>();
		inline static glm::vec2 s_MousePos = glm::vec2(0.f);
		inline static glm::vec2 s_MousePosPre = glm::vec2(0.f);
		inline static glm::vec2 s_MouseDelta = glm::vec2(0.f);
		inline static glm::vec2 s_ScrollDelta = glm::vec2(0.f);
	};
}
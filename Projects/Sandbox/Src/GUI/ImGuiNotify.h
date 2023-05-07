// Modified version of imgui-notify by patrickcjk
// https://github.com/patrickcjk/imgui-notify

/*
	Added features:
	* Option to turn off dismiss time by passing NOTIFY_NO_DISMISS.
	* Remove notification when right-clicked.
	* Remove all notifications when shift + right-clicked on any notification.
	* Can bind a callback and let it be signaled when left-clicked.
	* Flags, such as DismissOnSignal.
	* Option to change position of the toasts using ImGuiToastPos_*.
	* Bring to front (for some reason it was disabled).
**/

#ifndef IMGUI_NOTIFY
#define IMGUI_NOTIFY

#pragma once
#include <vector>
#include <string>
#include <imgui.h>
#include <functional>

#define NOTIFY_MAX_MSG_LENGTH			4096		// Max message content length
#define NOTIFY_PADDING_X				20.f		// Bottom-left X padding
#define NOTIFY_PADDING_Y				20.f		// Bottom-left Y padding
#define NOTIFY_PADDING_MESSAGE_Y		10.f		// Padding Y between each message
#define NOTIFY_FADE_IN_OUT_TIME			150			// Fade in and out duration
#define NOTIFY_DEFAULT_DISMISS			3000		// Auto dismiss after X ms (default, applied only of no data provided in constructors)
#define NOTIFY_OPACITY					1.0f		// 0-1 Toast opacity
#define NOTIFY_TOAST_FLAGS				ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing
// Comment out if you don't want any separator between title and content
//#define NOTIFY_USE_SEPARATOR

#define NOTIFY_NO_DISMISS				-1
#define NOTIFY_REMOVE_BUTTON			1 // Mouse buttons: 0=left, 1=right, 2=middle
#define NOTIFY_SIGNAL_BUTTON			0 // Mouse buttons: 0=left, 1=right, 2=middle

#define NOTIFY_INLINE					inline
#define NOTIFY_NULL_OR_EMPTY(str)		(!str ||! strlen(str))
#define NOTIFY_FORMAT(fn, format, ...)	if (format) { va_list args; va_start(args, format); fn(format, args, __VA_ARGS__); va_end(args); }

typedef int ImGuiToastType;
typedef int ImGuiToastPhase;
typedef int ImGuiToastPos;

enum ImGuiToastType_
{
	ImGuiToastType_None,
	ImGuiToastType_Success,
	ImGuiToastType_Warning,
	ImGuiToastType_Error,
	ImGuiToastType_Info,
	ImGuiToastType_COUNT
};

enum ImGuiToastPhase_
{
	ImGuiToastPhase_FadeIn,
	ImGuiToastPhase_Wait,
	ImGuiToastPhase_FadeOut,
	ImGuiToastPhase_Expired,
	ImGuiToastPhase_COUNT
};

enum ImGuiToastPos_
{
	ImGuiToastPos_TopLeft,
	ImGuiToastPos_TopCenter,
	ImGuiToastPos_TopRight,
	ImGuiToastPos_BottomLeft,
	ImGuiToastPos_BottomCenter,
	ImGuiToastPos_BottomRight,
	ImGuiToastPos_COUNT
};

enum ImGuiToastFlag_ : uint32_t
{
	ImGuiToastFlag_None = 0,
	ImGuiToastFlag_DismissOnSignal = 1
};

class ImGuiToast
{
private:
	ImGuiToastType				type = ImGuiToastType_None;
	char						title[NOTIFY_MAX_MSG_LENGTH];
	char						content[NOTIFY_MAX_MSG_LENGTH];
	int							dismiss_time = NOTIFY_DEFAULT_DISMISS;
	uint64_t					creation_time = 0;
	std::function<void(void)>	callback;
	uint32_t					flags = 0;

public:
	NOTIFY_INLINE auto signal() -> void { if (callback) callback(); }

	NOTIFY_INLINE ImGuiToast& bind(std::function<void(void)> callback) { this->callback = callback; return *this; }

	NOTIFY_INLINE ImGuiToast& dismissOnSignal() { flags |= ImGuiToastFlag_DismissOnSignal; return *this;}

private:
	// Setters

	NOTIFY_INLINE auto set_title_internal(const char* format, va_list args) { vsnprintf(this->title, sizeof(this->title), format, args); }

	NOTIFY_INLINE auto set_content_internal(const char* format, va_list args) { vsnprintf(this->content, sizeof(this->content), format, args); }

public:

	NOTIFY_INLINE auto set_title(const char* format, ...) -> void { NOTIFY_FORMAT(this->set_title_internal, format); }

	NOTIFY_INLINE auto set_content(const char* format, ...) -> void { NOTIFY_FORMAT(this->set_content_internal, format); }

	NOTIFY_INLINE auto set_type(const ImGuiToastType& type) -> void { IM_ASSERT(type < ImGuiToastType_COUNT); this->type = type; };

public:
	// Getters

	NOTIFY_INLINE auto get_title() -> char* { return this->title; };

	NOTIFY_INLINE auto get_default_title() -> char*
	{
		if (!strlen(this->title))
		{
			switch (this->type)
			{
			case ImGuiToastType_None:
				return NULL;
			case ImGuiToastType_Success:
				return "Success";
			case ImGuiToastType_Warning:
				return "Warning";
			case ImGuiToastType_Error:
				return "Error";
			case ImGuiToastType_Info:
				return "Info";
			}
		}

		return this->title;
	};

	NOTIFY_INLINE auto get_type() -> const ImGuiToastType& { return this->type; };

	NOTIFY_INLINE auto get_color() -> const ImVec4&
	{
		switch (this->type)
		{
		case ImGuiToastType_None:
			return { 255, 255, 255, 255 }; // White
		case ImGuiToastType_Success:
			return { 0, 255, 0, 255 }; // Green
		case ImGuiToastType_Warning:
			return { 255, 255, 0, 255 }; // Yellow
		case ImGuiToastType_Error:
			return { 255, 0, 0, 255 }; // Error
		case ImGuiToastType_Info:
			return { 0, 157, 255, 255 }; // Blue
		}
	}

	NOTIFY_INLINE auto get_icon() -> const char*
	{
		switch (this->type)
		{
		case ImGuiToastType_None:
			return NULL;
		case ImGuiToastType_Success:
			return u8"\uF058";
		case ImGuiToastType_Warning:
			return u8"\uF06A";
		case ImGuiToastType_Error:
			return u8"\uF057";
		case ImGuiToastType_Info:
			return u8"\uF059";
		}
	}

	NOTIFY_INLINE auto get_content() -> char* { return this->content; };

	NOTIFY_INLINE auto get_elapsed_time() { return GetTickCount64() - this->creation_time; }

	NOTIFY_INLINE auto get_phase() -> const ImGuiToastPhase&
	{
		const auto elapsed = get_elapsed_time();

		if (this->dismiss_time == NOTIFY_NO_DISMISS)
		{
			return ImGuiToastPhase_Wait;
		}
		else if (elapsed > NOTIFY_FADE_IN_OUT_TIME + this->dismiss_time + NOTIFY_FADE_IN_OUT_TIME)
		{
			return ImGuiToastPhase_Expired;
		}
		else if (elapsed > NOTIFY_FADE_IN_OUT_TIME + this->dismiss_time)
		{
			return ImGuiToastPhase_FadeOut;
		}
		else if (elapsed > NOTIFY_FADE_IN_OUT_TIME)
		{
			return ImGuiToastPhase_Wait;
		}
		else
		{
			return ImGuiToastPhase_FadeIn;
		}
	}

	NOTIFY_INLINE auto get_fade_percent() -> const float
	{
		const auto phase = get_phase();
		const auto elapsed = get_elapsed_time();

		if (phase == ImGuiToastPhase_FadeIn)
		{
			return ((float)elapsed / (float)NOTIFY_FADE_IN_OUT_TIME) * NOTIFY_OPACITY;
		}
		else if (phase == ImGuiToastPhase_FadeOut)
		{
			return (1.f - (((float)elapsed - (float)NOTIFY_FADE_IN_OUT_TIME - (float)this->dismiss_time) / (float)NOTIFY_FADE_IN_OUT_TIME)) * NOTIFY_OPACITY;
		}

		return 1.f * NOTIFY_OPACITY;
	}

	NOTIFY_INLINE auto get_flags() -> uint32_t { return flags; }

public:
	// Constructors

	ImGuiToast(ImGuiToastType type, int dismiss_time = NOTIFY_DEFAULT_DISMISS)
	{
		IM_ASSERT(type < ImGuiToastType_COUNT);

		this->type = type;
		this->dismiss_time = dismiss_time;
		this->creation_time = GetTickCount64();

		memset(this->title, 0, sizeof(this->title));
		memset(this->content, 0, sizeof(this->content));
	}

	ImGuiToast(ImGuiToastType type, const char* format, ...) : ImGuiToast(type) { NOTIFY_FORMAT(this->set_content, format); }

	ImGuiToast(ImGuiToastType type, int dismiss_time, const char* format, ...) : ImGuiToast(type, dismiss_time) { NOTIFY_FORMAT(this->set_content, format); }

};

namespace ImGui
{
	namespace _Internal
	{
		NOTIFY_INLINE uint32_t position = ImGuiToastPos_BottomRight;

		NOTIFY_INLINE ImVec2 GetWinPos(float height, const ImVec2& vpSize)
		{
			switch (position)
			{
			case ImGuiToastPos_TopLeft:
				return ImVec2(NOTIFY_PADDING_X, NOTIFY_PADDING_Y + height);
			case ImGuiToastPos_TopRight:
				return ImVec2(vpSize.x - NOTIFY_PADDING_X, NOTIFY_PADDING_Y + height);
			case ImGuiToastPos_TopCenter:
				return ImVec2(vpSize.x / 2 - NOTIFY_PADDING_X, NOTIFY_PADDING_Y + height);
			case ImGuiToastPos_BottomCenter:
				return ImVec2(vpSize.x / 2 - NOTIFY_PADDING_X, vpSize.y - NOTIFY_PADDING_Y - height);
			case ImGuiToastPos_BottomLeft:
				return ImVec2(NOTIFY_PADDING_X, vpSize.y - NOTIFY_PADDING_Y - height);
			case ImGuiToastPos_BottomRight:
			default:
				return ImVec2(vpSize.x - NOTIFY_PADDING_X, vpSize.y - NOTIFY_PADDING_Y - height);
			}
		}

		NOTIFY_INLINE ImVec2 GetWinPivot()
		{
			switch (position)
			{
			case ImGuiToastPos_TopLeft:
				return ImVec2(0.0f, 0.0f);
			case ImGuiToastPos_TopRight:
				return ImVec2(1.0f, 0.0f);
			case ImGuiToastPos_TopCenter:
				return ImVec2(0.5f, 0.0f);
			case ImGuiToastPos_BottomCenter:
				return ImVec2(0.5f, 1.0f);
			case ImGuiToastPos_BottomLeft:
				return ImVec2(0.0f, 1.0f);
			case ImGuiToastPos_BottomRight:
			default:
				return ImVec2(1.0f, 1.0f);
			}
		}
	}

	NOTIFY_INLINE std::vector<ImGuiToast> notifications;

	/// <summary>
	/// Insert a new toast in the list
	/// </summary>
	NOTIFY_INLINE ImGuiToast& InsertNotification(const ImGuiToast& toast)
	{
		notifications.push_back(toast);
		return notifications.back();
	}

	/// <summary>
	/// Remove a toast from the list by its index
	/// </summary>
	/// <param name="index">index of the toast to remove</param>
	NOTIFY_INLINE VOID RemoveNotification(int index)
	{
		notifications.erase(notifications.begin() + index);
	}

	NOTIFY_INLINE VOID SetToastPosition(uint32_t pos)
	{
		_Internal::position = pos;
	}

	/// <summary>
	/// Render toasts, call at the end of your rendering!
	/// </summary>
	NOTIFY_INLINE VOID RenderNotifications()
	{
		const auto vp_size = GetMainViewport()->Size;

		float height = 0.f;

		bool removeAllNotifications = false;
		for (auto i = 0; i < notifications.size(); i++)
		{
			auto* current_toast = &notifications[i];

			// Remove toast if expired
			if (current_toast->get_phase() == ImGuiToastPhase_Expired)
			{
				RemoveNotification(i);
				continue;
			}

			// Get icon, title and other data
			const auto icon = current_toast->get_icon();
			const auto title = current_toast->get_title();
			const auto content = current_toast->get_content();
			const auto default_title = current_toast->get_default_title();
			const auto opacity = current_toast->get_fade_percent(); // Get opacity based of the current phase

			// Window rendering
			auto text_color = current_toast->get_color();
			text_color.w = opacity;

			// Generate new unique name for this toast
			char window_name[50];
			sprintf_s(window_name, "##TOAST%d", i);

			//PushStyleColor(ImGuiCol_Text, text_color);
			SetNextWindowBgAlpha(opacity);

			ImVec2 winPos = _Internal::GetWinPos(height, vp_size);
			ImVec2 winPivot = _Internal::GetWinPivot();
			SetNextWindowPos(winPos, ImGuiCond_Always, winPivot);

			Begin(window_name, NULL, NOTIFY_TOAST_FLAGS);

			// Remove when clicked on.
			bool pendingRemoval = false;
			if (IsWindowHovered(ImGuiHoveredFlags_None))
			{
				if (IsMouseReleased(NOTIFY_REMOVE_BUTTON))
				{
					pendingRemoval = true;
					ImGuiIO& io = ImGui::GetIO();
					if (io.KeyShift)
						removeAllNotifications = true;
				}

				if (IsMouseReleased(NOTIFY_SIGNAL_BUTTON))
				{
					current_toast->signal();

					if (current_toast->get_flags())
						pendingRemoval = true;
				}
			}

			// Here we render the toast content
			{
				PushTextWrapPos(vp_size.x / 3.f); // We want to support multi-line text, this will wrap the text after 1/3 of the screen width

				bool was_title_rendered = false;

				// If an icon is set
				if (!NOTIFY_NULL_OR_EMPTY(icon))
				{
					//Text(icon); // Render icon text
					TextColored(text_color, icon);
					was_title_rendered = true;
				}

				// If a title is set
				if (!NOTIFY_NULL_OR_EMPTY(title))
				{
					// If a title and an icon is set, we want to render on same line
					if (!NOTIFY_NULL_OR_EMPTY(icon))
						SameLine();

					Text(title); // Render title text
					was_title_rendered = true;
				}
				else if (!NOTIFY_NULL_OR_EMPTY(default_title))
				{
					if (!NOTIFY_NULL_OR_EMPTY(icon))
						SameLine();

					Text(default_title); // Render default title text (ImGuiToastType_Success -> "Success", etc...)
					was_title_rendered = true;
				}

				// In case ANYTHING was rendered in the top, we want to add a small padding so the text (or icon) looks centered vertically
				if (was_title_rendered && !NOTIFY_NULL_OR_EMPTY(content))
				{
					SetCursorPosY(GetCursorPosY() + 5.f); // Must be a better way to do this!!!!
				}

				// If a content is set
				if (!NOTIFY_NULL_OR_EMPTY(content))
				{
					if (was_title_rendered)
					{
#ifdef NOTIFY_USE_SEPARATOR
						Separator();
#endif
					}

					Text(content); // Render content text
				}

				PopTextWrapPos();
			}

			// Save height for next toasts
			height += GetWindowHeight() + NOTIFY_PADDING_MESSAGE_Y;

			// End
			End();

			if (pendingRemoval)
			{
				RemoveNotification(i);
				continue;
			}
		}

		if (removeAllNotifications)
			notifications.clear();
	}
}

#endif
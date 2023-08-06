// Modified version of imgui-notify by patrickcjk
// https://github.com/patrickcjk/imgui-notify

/*
	Added features:
	* Option to turn off dismiss time by passing NOTIFY_NO_DISMISS.
	* Remove notification when right-clicked.
	* Remove all notifications when shift + right-clicked on any notification.
	* Can bind a callback and let it be signaled when left-clicked.
	* Flags, such as DismissOnSignal.
	* Option to change position of the notifications using ImGuiToastPos_*.
	* Bring to front (for some reason it was disabled).
	* Changed formatting to use Utils::Format (using fmt style formatting).
	* Changed functions to be templated instead of using va_list.
	* Made adding and removing notifications thread safe.
	* Added IMGUI prefix as to not interfere with other macros.
	* Added highlight when signaling or removing a notification.
**/

#ifndef IMGUI_NOTIFY
#define IMGUI_NOTIFY

#pragma once
#include <vector>
#include <string>
#include <imgui.h>
#include <functional>
#include <mutex>
#include <unordered_map>

#define IMGUI_NOTIFY_MAX_MSG_LENGTH			4096		// Max message content length
#define IMGUI_NOTIFY_PADDING_X				20.f		// Bottom-left X padding
#define IMGUI_NOTIFY_PADDING_Y				20.f		// Bottom-left Y padding
#define IMGUI_NOTIFY_PADDING_MESSAGE_Y		10.f		// Padding Y between each message
#define IMGUI_NOTIFY_FADE_IN_OUT_TIME		150			// Fade in and out duration
#define IMGUI_NOTIFY_DEFAULT_DISMISS		3000		// Auto dismiss after X ms (default, applied only of no data provided in constructors)
#define IMGUI_NOTIFY_OPACITY				1.0f		// 0-1 Toast opacity
#define IMGUI_NOTIFY_TOAST_FLAGS			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing
// Comment out if you don't want any separator between title and content
//#define IMGUI_NOTIFY_USE_SEPARATOR

#define IMGUI_NOTIFY_NO_DISMISS				-1
#define IMGUI_NOTIFY_REMOVE_BUTTON			1 // Mouse buttons: 0=left, 1=right, 2=middle
#define IMGUI_NOTIFY_SIGNAL_BUTTON			0 // Mouse buttons: 0=left, 1=right, 2=middle

#define IMGUI_NOTIFY_INLINE					inline
#define IMGUI_NOTIFY_NULL_OR_EMPTY(str)		(!str ||! strlen(str))

typedef int ImGuiToastType;
typedef int ImGuiToastPhase;
typedef int ImGuiToastPos;

enum ImGuiToastType_
{
	ImGuiToastType_None,
	ImGuiToastType_Success,
	ImGuiToastType_Warning,
	ImGuiToastType_Error,
	ImGuiToastType_Critical,
	ImGuiToastType_Info,
	ImGuiToastType_Debug,
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
public:
	using ID = uint64;

	ImGuiToast(const ImGuiToast& other)
		: type(other.type)
		, dismiss_time(other.dismiss_time)
		, creation_time(other.creation_time)
		, flags(other.flags)
		, id(other.id)
		, highlighted(other.highlighted)
	{
		assert(sizeof(this->title) == sizeof(other.title));
		assert(sizeof(this->content) == sizeof(other.content));
		memcpy(this->title, other.title, sizeof(this->title));
		memcpy(this->content, other.content, sizeof(this->content));

		this->callback = other.callback; // Lambda got corrupted. Is it because we are in another thread?
	}

private:
	ImGuiToastType				type = ImGuiToastType_None;
	char						title[IMGUI_NOTIFY_MAX_MSG_LENGTH];
	char						content[IMGUI_NOTIFY_MAX_MSG_LENGTH];
	int							dismiss_time = IMGUI_NOTIFY_DEFAULT_DISMISS;
	uint64_t					creation_time = 0;
	std::function<void(void)>	callback;
	uint32_t					flags = 0;
	ID							id = 0;
	bool						highlighted = false;

	IMGUI_NOTIFY_INLINE static uint64 generator = 0;
	IMGUI_NOTIFY_INLINE static std::mutex idGeneratorMutex;
public:
	IMGUI_NOTIFY_INLINE auto signal() -> void
	{
		if (callback)
			callback();
	}

	IMGUI_NOTIFY_INLINE ImGuiToast& bind(std::function<void(void)> callback)
	{
		this->callback = callback;
		return *this;
	}

	IMGUI_NOTIFY_INLINE ImGuiToast& dismissOnSignal()
	{
		flags |= ImGuiToastFlag_DismissOnSignal;
		return *this;
	}

	IMGUI_NOTIFY_INLINE bool is_highlighted() const { return this->highlighted; }

public:

	template<typename... Args>
	IMGUI_NOTIFY_INLINE auto set_title(const char* format, Args&&...args) -> void
	{
		std::string str = RS::Utils::Format(format, std::forward<Args>(args)...);
		strcpy(this->title, str.data());
	}

	template<typename... Args>
	IMGUI_NOTIFY_INLINE auto set_content(const char* format, Args&&...args) -> void
	{
		std::string str = RS::Utils::Format(format, std::forward<Args>(args)...);
		strcpy(this->content, str.data());
	}

	IMGUI_NOTIFY_INLINE auto set_type(const ImGuiToastType& type) -> void { IM_ASSERT(type < ImGuiToastType_COUNT); this->type = type; };

	IMGUI_NOTIFY_INLINE void set_highlighted(bool highlighted) { this->highlighted = highlighted; }

public:
	// Getters

	IMGUI_NOTIFY_INLINE auto get_title() -> char* { return this->title; };

	IMGUI_NOTIFY_INLINE auto get_default_title() -> char*
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
			case ImGuiToastType_Critical:
				return "Critical";
			case ImGuiToastType_Info:
				return "Info";
			case ImGuiToastType_Debug:
				return "Debug";
			}
		}

		return this->title;
	};

	IMGUI_NOTIFY_INLINE auto get_type() -> const ImGuiToastType& { return this->type; };

	IMGUI_NOTIFY_INLINE auto get_color() -> const ImVec4&
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
			return { 255, 0, 0, 255 }; // Red
		case ImGuiToastType_Critical:
			return { 127, 0, 0, 255 }; // Dark red
		case ImGuiToastType_Info:
			return { 0, 157, 255, 255 }; // Light Blue
		case ImGuiToastType_Debug:
			return { 0, 127, 0, 255 }; // Dark Green
		}
	}

	IMGUI_NOTIFY_INLINE auto get_icon() -> const char*
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
		case ImGuiToastType_Critical:
			return u8"\uF05B";
		case ImGuiToastType_Info:
			return u8"\uF059";
		case ImGuiToastType_Debug:
			return u8"\uF188";// u8"\uE490";
		}
	}

	IMGUI_NOTIFY_INLINE auto get_content() -> char* { return this->content; };

	IMGUI_NOTIFY_INLINE auto get_elapsed_time() { return GetTickCount64() - this->creation_time; }

	IMGUI_NOTIFY_INLINE auto get_phase() -> const ImGuiToastPhase&
	{
		const auto elapsed = get_elapsed_time();

		if (this->dismiss_time == IMGUI_NOTIFY_NO_DISMISS)
		{
			return ImGuiToastPhase_Wait;
		}
		else if (elapsed > IMGUI_NOTIFY_FADE_IN_OUT_TIME + this->dismiss_time + IMGUI_NOTIFY_FADE_IN_OUT_TIME)
		{
			return ImGuiToastPhase_Expired;
		}
		else if (elapsed > IMGUI_NOTIFY_FADE_IN_OUT_TIME + this->dismiss_time)
		{
			return ImGuiToastPhase_FadeOut;
		}
		else if (elapsed > IMGUI_NOTIFY_FADE_IN_OUT_TIME)
		{
			return ImGuiToastPhase_Wait;
		}
		else
		{
			return ImGuiToastPhase_FadeIn;
		}
	}

	IMGUI_NOTIFY_INLINE auto get_fade_percent() -> const float
	{
		const auto phase = get_phase();
		const auto elapsed = get_elapsed_time();

		if (phase == ImGuiToastPhase_FadeIn)
		{
			return ((float)elapsed / (float)IMGUI_NOTIFY_FADE_IN_OUT_TIME) * IMGUI_NOTIFY_OPACITY;
		}
		else if (phase == ImGuiToastPhase_FadeOut)
		{
			return (1.f - (((float)elapsed - (float)IMGUI_NOTIFY_FADE_IN_OUT_TIME - (float)this->dismiss_time) / (float)IMGUI_NOTIFY_FADE_IN_OUT_TIME)) * IMGUI_NOTIFY_OPACITY;
		}

		return 1.f * IMGUI_NOTIFY_OPACITY;
	}

	IMGUI_NOTIFY_INLINE auto get_flags() const -> uint32_t { return this->flags; }
	IMGUI_NOTIFY_INLINE auto get_id() const -> ID { return this->id; }

public:
	// Constructors
	ImGuiToast() // Added to make vector.reserve happy
	{
		this->type = ImGuiToastType_None;
		this->dismiss_time = IMGUI_NOTIFY_DEFAULT_DISMISS;
		this->creation_time = GetTickCount64();

		memset(this->title, 0, sizeof(this->title));
		memset(this->content, 0, sizeof(this->content));

		ID generatedID = 0;
		{
			std::lock_guard<std::mutex> lock(idGeneratorMutex);
			generatedID = ++this->generator;
		}
		this->id = generatedID;
	}

	ImGuiToast(ImGuiToastType type, int dismiss_time = IMGUI_NOTIFY_DEFAULT_DISMISS)
	{
		IM_ASSERT(type < ImGuiToastType_COUNT);

		this->type = type;
		this->dismiss_time = dismiss_time;
		this->creation_time = GetTickCount64();

		memset(this->title, 0, sizeof(this->title));
		memset(this->content, 0, sizeof(this->content));

		ID generatedID = 0;
		{
			std::lock_guard<std::mutex> lock(idGeneratorMutex);
			generatedID = ++this->generator;
		}
		this->id = generatedID;
	}

	template<typename... Args>
	ImGuiToast(ImGuiToastType type, const char* format, Args&&...args)
		: ImGuiToast(type)
	{ 
		this->dismiss_time = type == ImGuiToastType_Critical ? IMGUI_NOTIFY_NO_DISMISS : IMGUI_NOTIFY_DEFAULT_DISMISS;
		this->set_content(format, std::forward<Args>(args)...);
	}

	template<typename... Args>
	ImGuiToast(ImGuiToastType type, int dismiss_time, const char* format, Args&&...args)
		: ImGuiToast(type, dismiss_time)
	{
		this->set_content(format, std::forward<Args>(args)...);
	}

};

namespace ImGui
{
	IMGUI_NOTIFY_INLINE VOID HelpMarker(const char* desc)
	{
		ImGui::TextDisabled("(%s)", u8"\u003F");
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
		{
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	IMGUI_NOTIFY_INLINE uint32_t GetDefaultNotificationPosition() { return ImGuiToastPos_TopRight; }

	namespace _Internal
	{
		IMGUI_NOTIFY_INLINE bool hasToastNotifications = false;

		IMGUI_NOTIFY_INLINE uint32_t notificationPosition = GetDefaultNotificationPosition();

		IMGUI_NOTIFY_INLINE ImVec2 GetWinPos(float height, const ImVec2& vpSize)
		{
			switch (notificationPosition)
			{
			case ImGuiToastPos_TopLeft:
				return ImVec2(IMGUI_NOTIFY_PADDING_X, IMGUI_NOTIFY_PADDING_Y + height);
			case ImGuiToastPos_TopRight:
				return ImVec2(vpSize.x - IMGUI_NOTIFY_PADDING_X, IMGUI_NOTIFY_PADDING_Y + height);
			case ImGuiToastPos_TopCenter:
				return ImVec2(vpSize.x / 2 - IMGUI_NOTIFY_PADDING_X, IMGUI_NOTIFY_PADDING_Y + height);
			case ImGuiToastPos_BottomCenter:
				return ImVec2(vpSize.x / 2 - IMGUI_NOTIFY_PADDING_X, vpSize.y - IMGUI_NOTIFY_PADDING_Y - height);
			case ImGuiToastPos_BottomLeft:
				return ImVec2(IMGUI_NOTIFY_PADDING_X, vpSize.y - IMGUI_NOTIFY_PADDING_Y - height);
			case ImGuiToastPos_BottomRight:
			default:
				return ImVec2(vpSize.x - IMGUI_NOTIFY_PADDING_X, vpSize.y - IMGUI_NOTIFY_PADDING_Y - height);
			}
		}

		IMGUI_NOTIFY_INLINE ImVec2 GetWinPivot()
		{
			switch (notificationPosition)
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

	IMGUI_NOTIFY_INLINE bool HasToastNotifications()
	{
		return _Internal::hasToastNotifications;
	}

	IMGUI_NOTIFY_INLINE std::unordered_map<uint64, ImGuiToast> notifications;
	IMGUI_NOTIFY_INLINE std::vector<uint64> pendingNotificationRemovals;
	IMGUI_NOTIFY_INLINE std::mutex notificationsMutex;

	/// <summary>
	/// Insert a new toast in the list
	/// </summary>
	IMGUI_NOTIFY_INLINE ImGuiToast& InsertNotification(const ImGuiToast& toast)
	{
		std::lock_guard<std::mutex> lock(notificationsMutex);
		notifications.insert(std::pair<uint64, ImGuiToast>(toast.get_id(), toast));
		return notifications[toast.get_id()];
	}

	/// <summary>
	/// Remove a toast from the list by its ID
	/// </summary>
	/// <param name="id">ID of the toast to remove</param>
	IMGUI_NOTIFY_INLINE VOID RemoveNotification(ImGuiToast::ID id)
	{
		std::lock_guard<std::mutex> lock(notificationsMutex); // Change to a different mutex for removals?
		pendingNotificationRemovals.push_back(id);
	}

	/// <summary>
	/// Set the position of where the notifications appears.
	/// </summary>
	/// <param name="pos">position of the notification</param>
	IMGUI_NOTIFY_INLINE VOID SetNotificationPosition(uint32_t pos)
	{
		_Internal::notificationPosition = pos;
	}

	/// <summary>
	/// Render toasts, call at the end of your rendering!
	/// </summary>
	IMGUI_NOTIFY_INLINE VOID RenderNotifications()
	{
		const auto vp_size = GetMainViewport()->Size;

		float height = 0.f;

		std::vector<ImGuiToast> notificationsBuffer;
		{
			std::lock_guard<std::mutex> lock(notificationsMutex);
			notificationsBuffer.reserve(notifications.size());
			for (auto& pair : notifications)
				notificationsBuffer.push_back(pair.second);

			_Internal::hasToastNotifications = notifications.empty() == false;
		}

		bool removeAllNotifications = false;
		for (auto i = 0; i < notificationsBuffer.size(); i++)
		{
			bool is_dirty = false;
			auto* current_toast = &notificationsBuffer[i];

			// Remove toast if expired
			if (current_toast->get_phase() == ImGuiToastPhase_Expired)
			{
				RemoveNotification(current_toast->get_id());
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

			bool pendingHighlightedStylePop = false;
			if (current_toast->is_highlighted())
			{
				ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(127, 127, 127, 256));
				pendingHighlightedStylePop = true;
			}

			Begin(window_name, NULL, IMGUI_NOTIFY_TOAST_FLAGS);

			// Remove when clicked on.
			bool pendingRemoval = false;
			if (IsWindowHovered(ImGuiHoveredFlags_None))
			{
				// When pressing: highlight the notification in some way. Like change color, size, etc.

				if (IsMouseReleased(IMGUI_NOTIFY_REMOVE_BUTTON))
				{
					pendingRemoval = true;
					ImGuiIO& io = ImGui::GetIO();
					if (io.KeyShift)
						removeAllNotifications = true;
				}
				else if (IsMouseDown(IMGUI_NOTIFY_REMOVE_BUTTON))
				{
					current_toast->set_highlighted(true);
					is_dirty = true;
				}
				else if (IsMouseReleased(IMGUI_NOTIFY_SIGNAL_BUTTON))
				{
					current_toast->signal();

					if (current_toast->get_flags())
						pendingRemoval = true;
				}
				else if (IsMouseDown(IMGUI_NOTIFY_SIGNAL_BUTTON))
				{
					current_toast->set_highlighted(true);
					is_dirty = true;
				}
				// User had pressed down but moved outside the window and released, thus the highlight should also be removed.
				else if (current_toast->is_highlighted())
				{
					current_toast->set_highlighted(false);
					is_dirty = true;
				}
			}

			// Here we render the toast content
			{
				PushTextWrapPos(vp_size.x / 3.f); // We want to support multi-line text, this will wrap the text after 1/3 of the screen width

				bool was_title_rendered = false;

				// If an icon is set
				if (!IMGUI_NOTIFY_NULL_OR_EMPTY(icon))
				{
					//Text(icon); // Render icon text
					TextColored(text_color, icon);
					was_title_rendered = true;
				}

				// If a title is set
				if (!IMGUI_NOTIFY_NULL_OR_EMPTY(title))
				{
					// If a title and an icon is set, we want to render on same line
					if (!IMGUI_NOTIFY_NULL_OR_EMPTY(icon))
						SameLine();

					Text(title); // Render title text
					was_title_rendered = true;
				}
				else if (!IMGUI_NOTIFY_NULL_OR_EMPTY(default_title))
				{
					if (!IMGUI_NOTIFY_NULL_OR_EMPTY(icon))
						SameLine();

					Text(default_title); // Render default title text (ImGuiToastType_Success -> "Success", etc...)
					was_title_rendered = true;
				}

				// In case ANYTHING was rendered in the top, we want to add a small padding so the text (or icon) looks centered vertically
				if (was_title_rendered && !IMGUI_NOTIFY_NULL_OR_EMPTY(content))
				{
					SetCursorPosY(GetCursorPosY() + 5.f); // Must be a better way to do this!!!!
				}

				// If a content is set
				if (!IMGUI_NOTIFY_NULL_OR_EMPTY(content))
				{
					if (was_title_rendered)
					{
#ifdef IMGUI_NOTIFY_USE_SEPARATOR
						Separator();
#endif
					}

					Text(content); // Render content text
				}

				PopTextWrapPos();
			}

			// Save height for next toasts
			height += GetWindowHeight() + IMGUI_NOTIFY_PADDING_MESSAGE_Y;

			// End
			End();

			if (pendingHighlightedStylePop)
			{
				ImGui::PopStyleColor();
			}

			if (pendingRemoval)
			{
				RemoveNotification(current_toast->get_id());
				continue;
			}

			// If data is changed, update its original structs.
			// TODO: Might want to do this in bulk?
			if (is_dirty)
			{
				std::lock_guard<std::mutex> lock(notificationsMutex);
				memcpy(&notifications[current_toast->get_id()], current_toast, sizeof(ImGuiToast));
			}
		}

		{
			std::lock_guard<std::mutex> lock(notificationsMutex);
			if (removeAllNotifications)
			{
				notifications.clear();
				pendingNotificationRemovals.clear();
			}
			else
			{
				for (ImGuiToast::ID id : pendingNotificationRemovals)
					notifications.erase(id);
				pendingNotificationRemovals.clear();
			}
		}
	}
}

#endif
#pragma once

#include "GUI/ImGuiNotify.h"

#ifdef RS_CONFIG_DEVELOPMENT
#define NOTIFY_DETAILED(type, ...) ImGui::InsertNotification(ImGuiToast{ type, __VA_ARGS__ });
#define NOTIFY_INFO(...) NOTIFY_DETAILED(ImGuiToastType_Info, __VA_ARGS__)
#define NOTIFY_SUCCESS(...) NOTIFY_DETAILED(ImGuiToastType_Success, __VA_ARGS__)
#define NOTIFY_ERROR(...) NOTIFY_DETAILED(ImGuiToastType_Error, __VA_ARGS__)
#define NOTIFY_CRITICAL(...) NOTIFY_DETAILED(ImGuiToastType_Critical, __VA_ARGS__)
#define NOTIFY_DEBUG(...) NOTIFY_DETAILED(ImGuiToastType_Debug, __VA_ARGS__)
#else
#define NOTIFY_DETAILED(type, ...)
#define NOTIFY_INFO(...)
#define NOTIFY_SUCCESS(...)
#define NOTIFY_ERROR(...)
#define NOTIFY_CRITICAL(...)
#define NOTIFY_DEBUG(...)
#endif
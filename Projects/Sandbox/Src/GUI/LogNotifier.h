#pragma once

#include "GUI/ImGuiNotify.h"

#ifdef RS_CONFIG_DEVELOPMENT
#define RS_NOTIFY_DETAILED(type, ...) ImGui::InsertNotification(ImGuiToast{ type, __VA_ARGS__ });
#define RS_NOTIFY_INFO(...) RS_NOTIFY_DETAILED(ImGuiToastType_Info, __VA_ARGS__)
#define RS_NOTIFY_SUCCESS(...) RS_NOTIFY_DETAILED(ImGuiToastType_Success, __VA_ARGS__)
#define RS_NOTIFY_ERROR(...) RS_NOTIFY_DETAILED(ImGuiToastType_Error, __VA_ARGS__)
#define RS_NOTIFY_CRITICAL(...) RS_NOTIFY_DETAILED(ImGuiToastType_Critical, __VA_ARGS__)
#define RS_NOTIFY_DEBUG(...) RS_NOTIFY_DETAILED(ImGuiToastType_Debug, __VA_ARGS__)
#else
#define RS_NOTIFY_DETAILED(type, ...)
#define RS_NOTIFY_INFO(...)
#define RS_NOTIFY_SUCCESS(...)
#define RS_NOTIFY_ERROR(...)
#define RS_NOTIFY_CRITICAL(...)
#define RS_NOTIFY_DEBUG(...)
#endif
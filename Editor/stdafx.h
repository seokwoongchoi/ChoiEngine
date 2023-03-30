#pragma once

#define EDITORMODE
#include "Framework.h"
#pragma comment(lib, "Framework.lib")

#include <shellapi.h>
#include "./ImGui/Source/imgui.h"
#include "./ImGui/Source/imgui_internal.h"
#include "./ImGui/ImGuizmo.h"


#include "Debug/DebugLine.h"
#ifdef _DEBUG
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif



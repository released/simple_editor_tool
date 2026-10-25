#pragma once
#include <Windows.h>
#include <string>

namespace simple_editor::shell {
inline constexpr wchar_t kExplorerVerb[] = L"SimpleEditorTool.Open";
inline constexpr wchar_t kExplorerKey[] = L"*\\shell\\SimpleEditorTool.Open";
struct ExplorerMenuState { bool present = false, owned = false; std::wstring executable; };
bool ReadExplorerMenu(HKEY classes, ExplorerMenuState* state, std::wstring* error);
bool SetExplorerMenu(HKEY classes, const std::wstring& executable, bool enable, std::wstring* error);
bool ReadUserExplorerMenu(ExplorerMenuState* state, std::wstring* error);
bool SetUserExplorerMenu(const std::wstring& executable, bool enable, std::wstring* error);
std::wstring CurrentExecutable();
}

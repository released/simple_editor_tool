#include "explorer_menu.h"
#include <ShlObj.h>
#include <filesystem>
#include <vector>

namespace simple_editor::shell {
namespace {
constexpr wchar_t kOwnerName[] = L"SimpleEditorTool.Owner";
constexpr wchar_t kOwner[] = L"{0747285D-33D3-46F5-9152-1D4638D77A65}";
constexpr wchar_t kPathName[] = L"SimpleEditorTool.Executable";
struct Key { HKEY value = nullptr; ~Key() { if (value) RegCloseKey(value); } };
bool Error(LSTATUS code, std::wstring* error) {
    if (error) { wchar_t message[512]{}; FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,code,0,message,512,nullptr); *error = L"Explorer menu registry error " + std::to_wstring(code) + L": " + message; }
    return false;
}
struct Value {
    const wchar_t* name; bool exists = false; DWORD type = REG_SZ; std::vector<BYTE> data;
    bool Read(HKEY key, std::wstring* error) {
        DWORD bytes = 0; auto code = RegQueryValueExW(key,name,nullptr,&type,nullptr,&bytes);
        if (code == ERROR_FILE_NOT_FOUND) return true;
        if (code != ERROR_SUCCESS || bytes > 65536) return Error(code == ERROR_SUCCESS ? ERROR_MORE_DATA : code,error);
        data.resize(bytes); code = RegQueryValueExW(key,name,nullptr,&type,data.data(),&bytes);
        if (code != ERROR_SUCCESS) return Error(code,error); data.resize(bytes); exists = true; return true;
    }
    LSTATUS Restore(HKEY key) const { return exists ? RegSetValueExW(key,name,0,type,data.data(),static_cast<DWORD>(data.size())) : RegDeleteValueW(key,name); }
};
bool String(HKEY key, const wchar_t* name, std::wstring* text, std::wstring* error) {
    Value value{name}; if (!value.Read(key,error)) return false; text->clear(); if (!value.exists) return true;
    if (value.type != REG_SZ || value.data.size() < sizeof(wchar_t) || value.data.size() % sizeof(wchar_t)) return Error(ERROR_INVALID_DATA,error);
    const auto* first = reinterpret_cast<const wchar_t*>(value.data.data()); const auto count = value.data.size() / sizeof(wchar_t);
    if (first[count - 1] || wcsnlen_s(first,count) != count - 1) return Error(ERROR_INVALID_DATA,error); text->assign(first,count - 1); return true;
}
LSTATUS Put(HKEY key, const wchar_t* name, const std::wstring& value) { return RegSetValueExW(key,name,0,REG_SZ,reinterpret_cast<const BYTE*>(value.c_str()),static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t))); }
}
std::wstring CurrentExecutable() { std::vector<wchar_t> path(32768); const auto count = GetModuleFileNameW(nullptr,path.data(),static_cast<DWORD>(path.size())); return count && count < path.size() ? std::wstring(path.data(),count) : L""; }
bool ReadExplorerMenu(HKEY classes, ExplorerMenuState* state, std::wstring* error) {
    *state = {}; Key key; const auto code = RegOpenKeyExW(classes,kExplorerKey,0,KEY_QUERY_VALUE,&key.value);
    if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND) return true; if (code != ERROR_SUCCESS) return Error(code,error);
    state->present = true; std::wstring owner; if (!String(key.value,kOwnerName,&owner,error)) return false;
    state->owned = owner == kOwner; return !state->owned || String(key.value,kPathName,&state->executable,error);
}
bool SetExplorerMenu(HKEY classes, const std::wstring& executable, bool enable, std::wstring* error) {
    if (enable) { const auto attributes = GetFileAttributesW(executable.c_str()); if (executable.empty() || executable.size() > 30000 || executable.find_first_of(L"\"\r\n") != std::wstring::npos || executable.find(L'\0') != std::wstring::npos || !std::filesystem::path(executable).is_absolute() || _wcsicmp(std::filesystem::path(executable).extension().c_str(),L".exe") || attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) { if (error) *error = L"Choose an existing absolute executable path."; return false; } }
    ExplorerMenuState state; if (!ReadExplorerMenu(classes,&state,error)) return false;
    if (state.present && !state.owned) { if (error) *error = L"The named registry entry is not owned by Simple Editor Tool; it was retained."; return false; }
    if (!enable) { if (!state.present) return true; const auto code = RegDeleteTreeW(classes,kExplorerKey); return code == ERROR_SUCCESS || Error(code,error); }
    Key key,command; DWORD created = 0,command_created = 0;
    auto code = RegCreateKeyExW(classes,kExplorerKey,0,nullptr,0,KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY,nullptr,&key.value,&created);
    if (code != ERROR_SUCCESS) return Error(code,error);
    if (created == REG_OPENED_EXISTING_KEY) { std::wstring owner; if (!String(key.value,kOwnerName,&owner,error)) return false; if (owner != kOwner) { if (error) *error = L"The entry changed ownership; the registry was retained."; return false; } }
    Value old[] = {{L"MUIVerb"},{L"Icon"},{L"MultiSelectModel"},{kOwnerName},{kPathName}};
    for (auto& value : old) if (!value.Read(key.value,error)) { if (created == REG_CREATED_NEW_KEY) RegDeleteTreeW(classes,kExplorerKey); return false; }
    code = RegCreateKeyExW(key.value,L"command",0,nullptr,0,KEY_QUERY_VALUE | KEY_SET_VALUE,nullptr,&command.value,&command_created);
    Value old_command{nullptr};
    if (code == ERROR_SUCCESS && !old_command.Read(command.value,error)) { if (command_created == REG_CREATED_NEW_KEY) RegDeleteKeyW(key.value,L"command"); if (created == REG_CREATED_NEW_KEY) RegDeleteTreeW(classes,kExplorerKey); return false; }
    if (code == ERROR_SUCCESS) code = Put(command.value,nullptr,L"\"" + executable + L"\" \"%1\"");
    const std::wstring values[] = {L"\x4EE5 Simple Editor Tool \x958B\x555F",L"\"" + executable + L"\",0",L"Player",kOwner,executable};
    for (unsigned i = 0; code == ERROR_SUCCESS && i < 5; ++i) code = Put(key.value,old[i].name,values[i]);
    if (code == ERROR_SUCCESS) return true;
    Error(code,error); bool restored = true;
    if (created == REG_CREATED_NEW_KEY) restored = RegDeleteTreeW(classes,kExplorerKey) == ERROR_SUCCESS;
    else { for (const auto& value : old) { const auto result = value.Restore(key.value); restored = restored && (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND); } if (command.value) { const auto result = old_command.Restore(command.value); restored = restored && (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND); } if (command_created == REG_CREATED_NEW_KEY) RegDeleteKeyW(key.value,L"command"); }
    if (!restored && error) *error += L" The previous registration could not be fully restored."; return false;
}
bool ReadUserExplorerMenu(ExplorerMenuState* state, std::wstring* error) {
    Key key; const auto code = RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Classes",0,KEY_READ,&key.value);
    if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND) { *state = {}; return true; } return code == ERROR_SUCCESS ? ReadExplorerMenu(key.value,state,error) : Error(code,error);
}
bool SetUserExplorerMenu(const std::wstring& executable, bool enable, std::wstring* error) {
    Key key; const auto code = enable ? RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\Classes",0,nullptr,0,KEY_READ | KEY_WRITE | DELETE,nullptr,&key.value,nullptr) : RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Classes",0,KEY_READ | KEY_WRITE | DELETE,&key.value);
    if (!enable && (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND)) return true;
    if (code != ERROR_SUCCESS) return Error(code,error); if (!SetExplorerMenu(key.value,executable,enable,error)) return false;
    SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,nullptr,nullptr); return true;
}
}

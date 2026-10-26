#pragma once
#include "file_operations.h"
#include <array>
namespace simple_editor::core {
enum class ShortcutAction : unsigned { NewFile,OpenFiles,Save,Find,Replace,FindFiles,ReopenClosed,Count };
struct Shortcut { unsigned key = 0,modifiers = 0; };
struct AppPreferences {
    std::wstring font = L"Consolas",language = L"tw";
    std::wstring syntax_associations;
    unsigned font_tenths = 100,theme = 1,tab_width = 4;
    bool wrap = false,whitespace = false,use_tabs = false,auto_restore = false,recover_drafts = false;
    unsigned snapshot_seconds = 30;
    int zoom[3] = {100,100,100};
    std::vector<std::wstring> recent;
    std::array<Shortcut,static_cast<std::size_t>(ShortcutAction::Count)> shortcuts{{{'N',1},{'O',1},{'S',1},{'F',1},{'H',1},{'F',3},{'T',3}}};
};
bool ParseShortcut(const std::wstring& value,Shortcut* shortcut,std::wstring* error);
std::wstring FormatShortcut(const Shortcut& shortcut);
bool ValidatePreferences(const AppPreferences& preferences,std::wstring* error);
bool LoadPreferences(const std::filesystem::path& path,AppPreferences* preferences,std::wstring* error);
bool SavePreferences(const std::filesystem::path& path,const AppPreferences& preferences,std::wstring* error);
}

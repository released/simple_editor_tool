#pragma once
#include "folder_sync.h"
#include "line_merge.h"
namespace simple_editor::core {
struct FolderTextSettings { bool enabled = false; std::wstring include = L"*.txt;*.cpp;*.h;*.ini;*.log;*.csv;*.json"; TextEncoding encoding = TextEncoding::Auto; LineOptions options; };
bool ParseFolderTextSettings(const std::wstring&,FolderTextSettings*,std::wstring*);
bool EvaluateFolderText(const std::filesystem::path&,const std::filesystem::path&,const FolderTextSettings&,bool,const std::atomic_bool&,FolderRow*,std::wstring*);
}

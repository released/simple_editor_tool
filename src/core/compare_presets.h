#pragma once
#include "workspace_session.h"
namespace simple_editor::core {
struct ComparePreset { std::wstring name; WorkspaceEntry settings; };
WorkspaceEntry PresetSettings(const WorkspaceEntry& source);
bool LoadComparePresets(const std::filesystem::path&,std::vector<ComparePreset>*,std::wstring* error);
bool SaveComparePresets(const std::filesystem::path&,const std::vector<ComparePreset>&,std::wstring* error);
}

#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace simple_editor::core {
struct IndentationSettings { unsigned tab_width = 4,indent_width = 4; bool use_tabs = false; };
struct EditorConfigResult { IndentationSettings indentation; std::vector<std::wstring> warnings; unsigned files = 0; bool applied = false; };
EditorConfigResult ResolveEditorConfig(const std::filesystem::path& file,const IndentationSettings& defaults);
}

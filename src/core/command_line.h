#pragma once
#include "binary_document.h"
namespace simple_editor::core {
struct CommandLineResult { int exit_code = 2; bool help = false; std::uint64_t differences = 0; std::wstring message; };
std::wstring CommandLineHelp();
CommandLineResult CompareCommandLine(const std::vector<std::wstring>& arguments,const SaveControl& control = {});
}

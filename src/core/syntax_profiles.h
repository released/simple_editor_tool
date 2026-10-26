#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <vector>
namespace simple_editor::core {
struct SyntaxMode { std::wstring id,label; };
const std::vector<SyntaxMode>& SyntaxModes();
bool ValidSyntaxMode(const std::wstring&);
bool ParseSyntaxAssociations(const std::wstring&,std::map<std::wstring,std::wstring>*,std::wstring* error);
std::wstring ResolveSyntax(const std::filesystem::path&,const std::wstring& associations);
}

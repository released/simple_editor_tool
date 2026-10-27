#pragma once
#include "text_review.h"
#include <filesystem>
namespace simple_editor::core {
enum class OutlineLanguage { None,Cpp,Managed,Python,JavaScript,PowerShell,Shell,Batch,CMake,Sql };
bool CppOutlinePath(const std::filesystem::path&);
OutlineLanguage OutlineLanguageFor(const std::filesystem::path&,const std::string& lexer,bool explicit_cpp = false);
bool ProgramFunctionOutline(const std::wstring&,OutlineLanguage,const std::atomic_bool&,std::vector<MarkdownHeading>*,bool* truncated);
bool CppFunctionOutline(const std::wstring&,const std::atomic_bool&,std::vector<MarkdownHeading>*,bool* truncated,bool include_declarations = false);
}

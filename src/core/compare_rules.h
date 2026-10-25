#pragma once
#include <string>
#include <vector>
namespace simple_editor::core {
struct TextCompareRules { bool ignore_blank = false,mask_timestamps = false; std::vector<std::wstring> ignore_prefix,ignore_contains,mask_literal; };
bool ParseTextRules(const std::wstring& text,TextCompareRules* rules,std::wstring* error);
bool IgnoredLine(const std::wstring& text,const TextCompareRules& rules);
std::wstring MaskComparedText(const std::wstring& text,const TextCompareRules& rules);
}

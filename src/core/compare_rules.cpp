#include "compare_rules.h"
#include "line_merge.h"
#include "text_codec.h"
#include <algorithm>
#include <cwctype>
namespace simple_editor::core {
bool ParseTextRules(const std::wstring& text,TextCompareRules* output,std::wstring* error) {
    if (text.size() > 65536 || !ValidateText(text)) return false; TextCompareRules rules;
    for (const auto& line : SplitLines(text)) { if (line.text.empty() || line.text[0] == L'#') continue; const auto at = line.text.find(L'='); if (at == std::wstring::npos) { if (error) *error = L"Use setting=value on each line."; return false; } const auto key = line.text.substr(0,at),value = line.text.substr(at + 1);
        if (key == L"ignore-blank" || key == L"mask-timestamps") { if (value != L"yes" && value != L"no") { if (error) *error = L"Boolean rules accept yes or no."; return false; } (key == L"ignore-blank" ? rules.ignore_blank : rules.mask_timestamps) = value == L"yes"; }
        else { auto* list = key == L"ignore-prefix" ? &rules.ignore_prefix : key == L"ignore-contains" ? &rules.ignore_contains : key == L"mask" ? &rules.mask_literal : nullptr; if (!list || value.empty() || value.size() > 256 || rules.ignore_prefix.size() + rules.ignore_contains.size() + rules.mask_literal.size() >= 16) { if (error) *error = L"Rules: ignore-blank=yes/no, mask-timestamps=yes/no, ignore-prefix=text, ignore-contains=text, mask=text. Literal rules are case sensitive (16 rules, 256 characters each)."; return false; } list->push_back(value); }
    } *output = std::move(rules); return true;
}
bool IgnoredLine(const std::wstring& text,const TextCompareRules& rules) { if (rules.ignore_blank && std::all_of(text.begin(),text.end(),[](auto c) { return iswspace(c) != 0; })) return true; for (const auto& prefix : rules.ignore_prefix) if (text.compare(0,prefix.size(),prefix) == 0) return true; for (const auto& part : rules.ignore_contains) if (text.find(part) != std::wstring::npos) return true; return false; }
std::wstring MaskComparedText(const std::wstring& text,const TextCompareRules& rules) {
    auto result = text; for (const auto& part : rules.mask_literal) if (!part.empty()) for (std::size_t at = 0; (at = text.find(part,at)) != std::wstring::npos; at += part.size()) std::fill_n(result.begin() + at,part.size(),L'#');
    if (rules.mask_timestamps) for (std::size_t at = 0; at + 19 <= text.size(); ++at) { bool match = true; for (unsigned i = 0; i < 19; ++i) { const auto c = text[at + i]; if (i == 4 || i == 7) match = match && c == L'-'; else if (i == 10) match = match && (c == L'T' || c == L' '); else if (i == 13 || i == 16) match = match && c == L':'; else match = match && c >= L'0' && c <= L'9'; } if (match) { std::fill_n(result.begin() + at,19,L'#'); at += 18; } } return result;
}
}

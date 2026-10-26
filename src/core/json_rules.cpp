#include "json_rules.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
namespace simple_editor::core {
namespace {
bool Pointer(const std::wstring& text,JsonPointer* output) {
    if (text.size() > 512 || (!text.empty() && text.front() != L'/')) return false; JsonPointer tokens;
    for (std::size_t start = text.empty() ? text.size() : 1; start <= text.size() && !text.empty();) {
        const auto end = text.find(L'/',start); std::wstring token;
        for (auto at = start; at < (end == std::wstring::npos ? text.size() : end); ++at) { auto c = text[at]; if (c == L'~') { if (++at == text.size() || (text[at] != L'0' && text[at] != L'1')) return false; c = text[at] == L'0' ? L'~' : L'/'; } if (!c) return false; token += c; }
        tokens.push_back(std::move(token)); if (tokens.size() > 64) return false; if (end == std::wstring::npos) break; start = end + 1;
    } *output = std::move(tokens); return true;
}
}
bool ParseJsonRules(const std::wstring& definition,JsonCompareOptions* output,std::wstring* error) {
    if (definition.size() > 65536) { if (error) *error = L"JSON rules are limited to 65,536 characters."; return false; }
    JsonCompareOptions result; std::wistringstream lines(definition); std::wstring line; unsigned number = 0;
    while (std::getline(lines,line)) {
        ++number; const auto first = line.find_first_not_of(L" \t\r"); if (first == std::wstring::npos || line[first] == L'#') continue;
        std::wistringstream input(line); std::wstring operation,path,key,extra; JsonPointer pointer,relative;
        bool valid = static_cast<bool>(input >> operation >> std::quoted(path)) && Pointer(path,&pointer);
        if (valid && operation == L"ignore") { valid = !(input >> extra) && result.ignored.size() < 128; if (valid && std::find(result.ignored.begin(),result.ignored.end(),pointer) == result.ignored.end()) result.ignored.push_back(std::move(pointer)); }
        else if (valid && operation == L"key") { valid = static_cast<bool>(input >> std::quoted(key)) && !(input >> extra) && Pointer(key,&relative) && result.arrays.size() < 32; for (const auto& array : result.arrays) if (array.path == pointer) valid = false; if (valid) result.arrays.push_back({std::move(pointer),std::move(relative)}); }
        else valid = false;
        if (!valid) { if (error) *error = L"Invalid JSON rule at line " + std::to_wstring(number) + L". Use ignore /path or key /array /id; quoted pointers may contain spaces. RFC 6901 ~0 and ~1 escapes; up to 128 ignores / 32 array keys."; return false; }
    }
    for (const auto& rule : result.arrays) for (const auto& ignored : result.ignored) if (ignored.size() > rule.path.size() && std::equal(rule.path.begin(),rule.path.end(),ignored.begin())) { if (error) *error = L"Ignore pointers below a keyed array are ambiguous after reordering. Ignore the array itself or use pointers outside it."; return false; }
    for (const auto& parent : result.arrays) for (const auto& child : result.arrays) if (child.path.size() > parent.path.size() && std::equal(parent.path.begin(),parent.path.end(),child.path.begin())) { if (error) *error = L"Nested key rules beneath a reordered keyed array are ambiguous; configure independent array containers."; return false; }
    *output = std::move(result); return true;
}
}

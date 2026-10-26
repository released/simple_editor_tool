#include "syntax_profiles.h"
#include "line_merge.h"
#include "text_codec.h"
#include <cwctype>
namespace simple_editor::core {
namespace {
std::wstring Lower(std::wstring value) { for (auto& c : value) c = static_cast<wchar_t>(towlower(c)); return value; }
std::wstring Trim(const std::wstring& value) { const auto begin = value.find_first_not_of(L" \t"); return begin == std::wstring::npos ? L"" : value.substr(begin,value.find_last_not_of(L" \t") - begin + 1); }
}
const std::vector<SyntaxMode>& SyntaxModes() {
    static const std::vector<SyntaxMode> modes{{L"",L"Automatic"},{L"text",L"Plain text"},{L"cpp",L"C/C++"},{L"python",L"Python"},{L"json",L"JSON"},{L"hypertext",L"HTML/XML"},{L"powershell",L"PowerShell"},{L"markdown",L"Markdown"},{L"sql",L"SQL"},{L"yaml",L"YAML"},{L"bash",L"Shell"},{L"batch",L"Batch"},{L"diff",L"Diff/Patch"},{L"cmake",L"CMake"}}; return modes;
}
bool ValidSyntaxMode(const std::wstring& name) { for (const auto& mode : SyntaxModes()) if (mode.id == name) return true; return false; }
bool ParseSyntaxAssociations(const std::wstring& text,std::map<std::wstring,std::wstring>* output,std::wstring* error) {
    const auto fail = [&]() { if (error) *error = L"Use one extension=language per line (for example .conf=JSON). Choose a supported language, unique extensions and at most 128 mappings."; return false; };
    if (!output || text.size() > 16384 || !ValidateText(text,error)) return fail(); std::map<std::wstring,std::wstring> result;
    for (const auto& line : SplitLines(text)) { const auto value = Trim(line.text); if (value.empty() || value[0] == L'#') continue; const auto at = value.find(L'='); if (at == std::wstring::npos) return fail(); const auto extension = Lower(Trim(value.substr(0,at))),name = Lower(Trim(value.substr(at + 1)));
        if (extension.size() < 2 || extension.size() > 32 || extension.front() != L'.' || extension.substr(1).find_first_of(L". \\/:*?\"<>|\t") != std::wstring::npos || result.size() >= 128) return fail(); bool known = false;
        for (const auto& mode : SyntaxModes()) if (name == Lower(mode.label) || (!mode.id.empty() && name == mode.id)) { if (!result.emplace(extension,mode.id).second) return fail(); known = true; break; } if (!known) return fail();
    } *output = std::move(result); return true;
}
std::wstring ResolveSyntax(const std::filesystem::path& path,const std::wstring& text) { std::map<std::wstring,std::wstring> mappings; std::wstring error; if (!ParseSyntaxAssociations(text,&mappings,&error)) return L""; const auto found = mappings.find(Lower(path.extension().wstring())); return found == mappings.end() ? L"" : found->second; }
}

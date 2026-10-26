#include "editor_config.h"
#include "binary_document.h"
#include "line_merge.h"
#include "text_codec.h"
#include <algorithm>
#include <map>
#include <string_view>
namespace simple_editor::core {
namespace {
std::wstring TrimConfig(std::wstring value) { const auto first = value.find_first_not_of(L" \t"),last = value.find_last_not_of(L" \t"); return first == std::wstring::npos ? L"" : value.substr(first,last - first + 1); }
std::wstring LowerConfig(std::wstring value) { for (auto& ch : value) if (ch >= L'A' && ch <= L'Z') ch += L'a' - L'A'; return value; }
bool Width(const std::wstring& value,unsigned* width) { if (value.empty() || value.size() > 2 || value.find_first_not_of(L"0123456789") != std::wstring::npos) return false; const auto number = std::stoul(value); if (!number || number > 16) return false; *width = static_cast<unsigned>(number); return true; }
bool MatchConfig(std::wstring pattern,const std::wstring& text,std::size_t* budget) {
    if (!pattern.empty() && pattern.front() == L'/') pattern.erase(0,1);
    std::vector<unsigned char> before(text.size() + 1),after(text.size() + 1); before[0] = 1;
    for (std::size_t p = 0; p < pattern.size(); ++p) {
        const auto ch = pattern[p]; const bool star = ch == L'*',globstar = star && p + 1 < pattern.size() && pattern[p + 1] == L'*'; if (globstar) ++p;
        const auto zero_directories = before;
        if (*budget < text.size() + 1) { *budget = 0; return false; } *budget -= text.size() + 1;
        std::fill(after.begin(),after.end(),static_cast<unsigned char>(0)); after[0] = star ? before[0] : 0;
        for (std::size_t t = 1; t <= text.size(); ++t) after[t] = star ? before[t] || (after[t - 1] && (globstar || text[t - 1] != L'/')) : before[t - 1] && (ch == text[t - 1] || (ch == L'?' && text[t - 1] != L'/'));
        before.swap(after);
        if (globstar && p + 1 < pattern.size() && pattern[p + 1] == L'/') {
            // A double-star directory prefix may match zero complete directories.
            ++p; std::fill(after.begin(),after.end(),static_cast<unsigned char>(0));
            for (std::size_t t = 1; t <= text.size(); ++t) after[t] = before[t - 1] && text[t - 1] == L'/';
            for (std::size_t t = 0; t <= text.size(); ++t) after[t] = after[t] || zero_directories[t]; before.swap(after);
        }
    } return before.back() != 0;
}
}
EditorConfigResult ResolveEditorConfig(const std::filesystem::path& requested,const IndentationSettings& defaults) {
    EditorConfigResult result; result.indentation = defaults; const auto warn = [&](std::wstring text) { if (result.warnings.size() < 31) result.warnings.push_back(std::move(text)); else if (result.warnings.size() == 31) result.warnings.push_back(L"Further EditorConfig diagnostics omitted."); }; std::error_code code; const auto file = std::filesystem::absolute(requested,code).lexically_normal(); if (code) { warn(L"Cannot resolve the document path."); return result; }
    if (file.native().rfind(L"\\\\",0) == 0) { warn(L"EditorConfig subset lookup is limited to local file paths."); return result; }
    struct Config { std::filesystem::path directory; std::wstring text; }; std::vector<Config> configs;
    auto directory = file.parent_path(); bool root = false;
    for (unsigned depth = 0; depth < 32 && !directory.empty() && !root; ++depth) {
        const auto path = directory / L".editorconfig"; const auto attributes = GetFileAttributesW(path.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES) {
            if (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) warn(L"Skipped non-regular EditorConfig: " + path.wstring());
            else {
                BinaryDocument document; std::wstring error,text; TextCodec codec; std::vector<std::uint8_t> bytes;
                if (!document.Open(path.wstring(),&error) || document.Size() > 65536 || !document.Read(0,static_cast<std::size_t>(document.Size()),&bytes) || !DecodeText(bytes,TextEncoding::Utf8,&text,&codec,&error) || !document.ValidateSource(true,&error)) warn(L"Skipped unreadable, invalid UTF-8 or oversized EditorConfig: " + path.wstring());
                else {
                    bool section = false; for (const auto& line : SplitLines(text)) { const auto value = TrimConfig(line.text); if (value.empty() || value[0] == L'#' || value[0] == L';') continue; if (value[0] == L'[') section = true; if (!section) { const auto at = value.find(L'='); if (at != std::wstring::npos && LowerConfig(TrimConfig(value.substr(0,at))) == L"root" && LowerConfig(TrimConfig(value.substr(at + 1))) == L"true") root = true; } }
                    configs.push_back({directory,std::move(text)}); ++result.files;
                }
            }
        } else if (GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND) warn(L"Cannot inspect EditorConfig: " + path.wstring());
        const auto parent = directory.parent_path(); if (parent == directory) break; directory = parent;
        if (depth == 31 && !root) warn(L"EditorConfig lookup stopped at 32 parent directories.");
    }
    std::map<std::wstring,std::wstring> values; std::size_t budget = 8 * 1024 * 1024;
    for (auto config = configs.rbegin(); config != configs.rend(); ++config) {
        bool match = false; unsigned sections = 0; auto relative = file.lexically_relative(config->directory).generic_wstring();
        for (const auto& item : SplitLines(config->text)) {
            auto line = TrimConfig(item.text); if (line.empty() || line[0] == L'#' || line[0] == L';') continue;
            if (line[0] == L'[') {
                match = false; if (++sections > 128 || relative.size() > 4096 || line.size() < 3 || line.back() != L']') { warn(L"Skipped an invalid or over-limit EditorConfig section."); continue; }
                auto pattern = line.substr(1,line.size() - 2); if (pattern.size() > 256 || pattern.find_first_of(L"[]{}\\") != std::wstring::npos) { warn(L"EditorConfig subset supports *, ** and ? sections; skipped unsupported glob: " + pattern.substr(0,128)); continue; }
                match = MatchConfig(pattern,pattern.find(L'/') == std::wstring::npos ? file.filename().wstring() : relative,&budget);
                if (!budget) { warn(L"EditorConfig glob work limit reached; remaining sections ignored."); break; } continue;
            }
            const auto at = line.find(L'='); if (!match || at == std::wstring::npos) continue;
            auto key = LowerConfig(TrimConfig(line.substr(0,at))),value = LowerConfig(TrimConfig(line.substr(at + 1)));
            if (key != L"indent_style" && key != L"indent_size" && key != L"tab_width") { warn(L"EditorConfig subset does not apply: " + key.substr(0,128)); continue; }
            unsigned width = 0; const bool valid = value == L"unset" || (key == L"indent_style" ? value == L"tab" || value == L"space" : key == L"indent_size" && value == L"tab" ? true : Width(value,&width));
            if (!valid) { warn(L"Invalid EditorConfig indentation value: " + key); continue; }
            if (value == L"unset") values.erase(key); else values[key] = value;
        }
    }
    auto settings = defaults; const auto style = values.find(L"indent_style"); if (style != values.end()) settings.use_tabs = style->second == L"tab";
    const auto size = values.find(L"indent_size"),tab = values.find(L"tab_width"); unsigned number = 0;
    if (tab != values.end() && Width(tab->second,&number)) settings.tab_width = number;
    if (size != values.end()) { if (size->second == L"tab") settings.indent_width = settings.tab_width; else if (Width(size->second,&number)) { settings.indent_width = number; if (tab == values.end()) settings.tab_width = number; } }
    else settings.indent_width = settings.tab_width;
    result.indentation = settings; result.applied = !values.empty(); if (result.warnings.size() > 32) { result.warnings.resize(32); result.warnings.back() = L"Further EditorConfig diagnostics omitted."; } return result;
}
}

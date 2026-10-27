#include "function_outline.h"
#include "text_codec.h"
#include <algorithm>
#include <cwctype>
#include <set>
namespace simple_editor::core {
OutlineLanguage OutlineLanguageFor(const std::filesystem::path& path,const std::string& lexer,bool explicit_cpp) {
    auto ext = path.extension().wstring(); for (auto& c : ext) c = static_cast<wchar_t>(towlower(c));
    if (lexer == "python") return OutlineLanguage::Python; if (lexer == "powershell") return OutlineLanguage::PowerShell; if (lexer == "bash") return OutlineLanguage::Shell; if (lexer == "batch") return OutlineLanguage::Batch; if (lexer == "cmake") return OutlineLanguage::CMake; if (lexer == "sql") return OutlineLanguage::Sql;
    if (lexer != "cpp") return OutlineLanguage::None; if (explicit_cpp || CppOutlinePath(path)) return OutlineLanguage::Cpp;
    if (ext == L".cs" || ext == L".java") return OutlineLanguage::Managed;
    if (ext == L".js" || ext == L".jsx" || ext == L".mjs" || ext == L".cjs" || ext == L".ts" || ext == L".tsx") return OutlineLanguage::JavaScript;
    return OutlineLanguage::None;
}
namespace {
std::wstring Lower(std::wstring value) { for (auto& c : value) c = static_cast<wchar_t>(towlower(c)); return value; }
bool Name(wchar_t c) { return iswalnum(c) || c == L'_' || c == L'$'; }
bool Identifier(const std::wstring& value) { return !value.empty() && (iswalpha(value[0]) || value[0] == L'_' || value[0] == L'$'); }
bool Mask(const std::wstring& text,OutlineLanguage language,const std::atomic_bool& cancel,std::wstring* output) {
    *output = text; auto& mask = *output; const auto erase = [&](std::size_t begin,std::size_t end) { for (auto n = begin; n < end; ++n) if (text[n] != L'\r' && text[n] != L'\n') mask[n] = L' '; };
    const bool hash = language == OutlineLanguage::Python || language == OutlineLanguage::PowerShell || language == OutlineLanguage::Shell || language == OutlineLanguage::CMake;
    for (std::size_t i = 0; i < text.size();) {
        if (cancel) return false; const auto begin = i; const auto c = text[i]; const wchar_t next = i + 1 < text.size() ? text[i + 1] : L'\0';
        if (language == OutlineLanguage::CMake && c == L'#' && next == L'[') { auto at = i + 2; while (at < text.size() && text[at] == L'=' && at - i <= 17) ++at; if (at < text.size() && text[at] == L'[') { const auto close = L"]" + text.substr(i + 2,at - i - 2) + L"]"; const auto end = text.find(close,at + 1); i = end == std::wstring::npos ? text.size() : end + close.size(); erase(begin,i); continue; } }
        if (language == OutlineLanguage::Shell && c == L'<' && next == L'<') { auto at = i + 2; if (at < text.size() && text[at] == L'-') ++at; while (at < text.size() && (text[at] == L' ' || text[at] == L'\t')) ++at; const auto quote = at < text.size() && (text[at] == L'\'' || text[at] == L'"') ? text[at++] : 0; const auto name = at; while (at < text.size() && Name(text[at]) && at - name < 64) ++at; const auto delimiter = text.substr(name,at - name); if (!delimiter.empty()) { auto row = text.find(L'\n',at); while (row != std::wstring::npos && row + 1 < text.size()) { if (cancel) return false; const auto end = text.find(L'\n',row + 1); auto content = text.substr(row + 1,(end == std::wstring::npos ? text.size() : end) - row - 1); const auto left = content.find_first_not_of(L"\t"); if (left != std::wstring::npos) content.erase(0,left); if (!content.empty() && content.back() == L'\r') content.pop_back(); if (content == delimiter) { i = end == std::wstring::npos ? text.size() : end; erase(begin,i); break; } row = end; } if (i != begin) continue; } (void)quote; }
        if ((hash && c == L'#') || (language == OutlineLanguage::JavaScript && c == L'/' && next == L'/') || (language == OutlineLanguage::Sql && c == L'-' && next == L'-')) { while (i < text.size() && text[i] != L'\n' && text[i] != L'\r') ++i; erase(begin,i); continue; }
        if ((language == OutlineLanguage::JavaScript || language == OutlineLanguage::Sql) && c == L'/' && next == L'*') { i += 2; while (i + 1 < text.size() && !(text[i] == L'*' && text[i + 1] == L'/')) { if (cancel) return false; ++i; } i = (std::min)(text.size(),i + 2); erase(begin,i); continue; }
        if (language == OutlineLanguage::PowerShell && c == L'<' && next == L'#') { const auto end = text.find(L"#>",i + 2); i = end == std::wstring::npos ? text.size() : end + 2; erase(begin,i); continue; }
        if (language == OutlineLanguage::CMake && c == L'[') { auto at = i + 1; while (at < text.size() && text[at] == L'=' && at - i <= 16) ++at; if (at < text.size() && text[at] == L'[') { const auto close = L"]" + text.substr(i + 1,at - i - 1) + L"]"; const auto end = text.find(close,at + 1); i = end == std::wstring::npos ? text.size() : end + close.size(); erase(begin,i); continue; } }
        if (language == OutlineLanguage::PowerShell && c == L'@' && (next == L'\'' || next == L'"') && i + 2 < text.size() && (text[i + 2] == L'\r' || text[i + 2] == L'\n')) { const auto end = text.find(L"\n" + std::wstring(1,next) + L"@",i + 2); i = end == std::wstring::npos ? text.size() : end + 3; erase(begin,i); continue; }
        if (c == L'\'' || c == L'"' || (language == OutlineLanguage::JavaScript && c == L'`')) {
            const bool triple = language == OutlineLanguage::Python && i + 2 < text.size() && text[i + 1] == c && text[i + 2] == c; i += triple ? 3 : 1;
            while (i < text.size()) { if (cancel) return false; if (text[i] == L'\\' || (language == OutlineLanguage::PowerShell && text[i] == L'`')) { i = (std::min)(text.size(),i + 2); continue; } if (text[i] == c && (!triple || (i + 2 < text.size() && text[i + 1] == c && text[i + 2] == c))) { i += triple ? 3 : 1; break; } ++i; }
            erase(begin,i); continue;
        }
        if (language == OutlineLanguage::JavaScript && c == L'/') {
            auto previous = i; while (previous && iswspace(text[previous - 1])) --previous; const bool regexp = !previous || wcschr(L"=([{!:,;?",text[previous - 1]) || (previous >= 6 && text.substr(previous - 6,6) == L"return");
            if (regexp) { ++i; bool bracket = false; while (i < text.size() && text[i] != L'\n' && text[i] != L'\r') { if (text[i] == L'\\') { i = (std::min)(text.size(),i + 2); continue; } if (text[i] == L'[') bracket = true; if (text[i] == L']') bracket = false; if (text[i++] == L'/' && !bracket) break; } while (i < text.size() && iswalpha(text[i])) ++i; erase(begin,i); continue; }
        }
        ++i;
    } return !cancel;
}
struct Token { std::wstring value; std::size_t at,line; bool first; };
}
bool ProgramFunctionOutline(const std::wstring& text,OutlineLanguage language,const std::atomic_bool& cancel,std::vector<MarkdownHeading>* output,bool* truncated) {
    if (!output || !truncated || language == OutlineLanguage::None || text.size() > 32 * 1024 * 1024 || cancel || !ValidateText(text)) return false;
    if (language == OutlineLanguage::Cpp || language == OutlineLanguage::Managed) return CppFunctionOutline(text,cancel,output,truncated,true);
    std::wstring masked; if (!Mask(text,language,cancel,&masked)) return false;
    std::vector<Token> tokens; std::size_t i = 0,line = 1; bool first = true,limited = false;
    while (i < masked.size()) {
        if (cancel) return false; const auto c = masked[i]; if (c == L'\r' || c == L'\n') { if (c == L'\r' && i + 1 < masked.size() && masked[i + 1] == L'\n') ++i; ++i; ++line; first = true; continue; } if (iswspace(c)) { ++i; continue; }
        const auto at = i; if (Name(c)) { ++i; while (i < masked.size() && Name(masked[i])) ++i; } else ++i;
        if (tokens.size() == 1000000) { limited = true; break; } tokens.push_back({masked.substr(at,(std::min<std::size_t>)(160,i - at)),at,line,first}); first = false;
    }
    std::vector<std::size_t> closing(tokens.size(),SIZE_MAX),stack; for (std::size_t n = 0; n < tokens.size(); ++n) { if (cancel) return false; if (tokens[n].value == L"(") { if (stack.size() == 1024) { limited = true; break; } stack.push_back(n); } else if (tokens[n].value == L")" && !stack.empty()) { closing[stack.back()] = n; stack.pop_back(); } }
    std::vector<MarkdownHeading> result; std::set<std::size_t> seen,body_braces; std::vector<unsigned> scopes; unsigned class_depth = 0,function_depth = 0; bool pending_class = false; std::size_t work = 0;
    const auto add = [&](std::size_t index) { if (index >= tokens.size() || !seen.insert(tokens[index].at).second) return; if (result.size() == 4096) { limited = true; return; } const auto at = tokens[index].at; const auto end = text.find_first_of(L"\r\n",at); auto title = text.substr(at,(std::min<std::size_t>)(160,end == std::wstring::npos ? text.size() - at : end - at)); if (!title.empty() && title.back() >= 0xD800 && title.back() <= 0xDBFF) title.pop_back(); result.push_back({std::move(title),tokens[index].line,1}); };
    const auto value = [&](std::size_t n) -> std::wstring { return n < tokens.size() ? tokens[n].value : L""; };
    const auto body_after = [&](std::size_t n) { for (unsigned steps = 0; n < tokens.size() && steps < 128; ++n,++steps) { ++work; if (value(n) == L"{" || value(n) == L";") return n; if (value(n) == L"=" || value(n) == L"}") break; } return SIZE_MAX; };
    static const std::set<std::wstring> controls{L"if",L"for",L"while",L"switch",L"catch",L"with",L"function",L"return",L"new",L"super"};
    for (std::size_t n = 0; n < tokens.size(); ++n) {
        if (cancel) return false; if (++work > 2000000) { limited = true; break; } const auto word = value(n),lower = Lower(word);
        if (language == OutlineLanguage::Python && word == L"def" && (tokens[n].first || (n && value(n - 1) == L"async")) && Identifier(value(n + 1)) && value(n + 2) == L"(") add(n + 1);
        else if (language == OutlineLanguage::PowerShell && (lower == L"function" || lower == L"filter") && Identifier(value(n + 1))) { for (auto next = n + 2; next < tokens.size() && next < n + 128; ++next) { if (value(next) == L"{") { add(n); break; } if (value(next) == L";") break; } }
        else if (language == OutlineLanguage::Shell) { if (word == L"function" && Identifier(value(n + 1))) add(n + 1); else if (Identifier(word) && value(n + 1) == L"(" && value(n + 2) == L")" && value(n + 3) == L"{") add(n); }
        else if (language == OutlineLanguage::CMake && tokens[n].first && (lower == L"function" || lower == L"macro") && value(n + 1) == L"(" && Identifier(value(n + 2))) add(n + 2);
        else if (language == OutlineLanguage::Batch && tokens[n].first && word == L":" && Identifier(value(n + 1)) && Lower(value(n + 1)) != L"eof") add(n + 1);
        else if (language == OutlineLanguage::Sql && lower == L"create") { auto next = n + 1; if (Lower(value(next)) == L"or" && (Lower(value(next + 1)) == L"replace" || Lower(value(next + 1)) == L"alter")) next += 2; const auto kind = Lower(value(next)); if ((kind == L"function" || kind == L"procedure" || kind == L"proc") && Identifier(value(next + 1))) add(next + 1); }
        else if (language == OutlineLanguage::JavaScript) {
            if (word == L"class" || word == L"interface") pending_class = true;
            if (word == L"{") { const auto type = pending_class ? 1u : body_braces.count(n) ? 2u : 0u; scopes.push_back(type); if (type == 1) ++class_depth; if (type == 2) ++function_depth; pending_class = false; }
            if (word == L"}" && !scopes.empty()) { if (scopes.back() == 1) --class_depth; if (scopes.back() == 2) --function_depth; scopes.pop_back(); }
            if (word == L"function") { auto name = n + 1; if (value(name) == L"*") ++name; if (Identifier(value(name)) && value(name + 1) == L"(" && closing[name + 1] != SIZE_MAX) { const auto body = body_after(closing[name + 1] + 1); if (body != SIZE_MAX) { add(name); if (value(body) == L"{") body_braces.insert(body); } } }
            if (word == L"const" || word == L"let" || word == L"var") { const auto name = n + 1; if (Identifier(value(name))) { auto next = name + 1; while (next < tokens.size() && next < n + 64 && value(next) != L"=" && value(next) != L";" && value(next) != L"{") ++next; if (value(next) == L"=") { ++next; if (value(next) == L"async") ++next; const bool anonymous = value(next) == L"function"; if (anonymous) ++next; if (value(next) == L"(" && closing[next] != SIZE_MAX) next = closing[next] + 1; else if (Identifier(value(next))) ++next; if (value(next) == L":") { const auto end = (std::min)(tokens.size(),next + 128); while (next < end && value(next) != L"=" && value(next) != L";" && value(next) != L"{") ++next; } if ((value(next) == L"=" && value(next + 1) == L">") || (anonymous && value(next) == L"{")) { add(name); if (!anonymous) next += 2; if (value(next) == L"{") body_braces.insert(next); } } } }
            if (Identifier(word) && !controls.count(word) && value(n + 1) == L"(" && closing[n + 1] != SIZE_MAX) { const auto previous = n ? value(n - 1) : L"{"; const bool boundary = previous == L"{" || previous == L"}" || previous == L";" || previous == L"static" || previous == L"async" || previous == L"get" || previous == L"set" || previous == L"public" || previous == L"private" || previous == L"protected" || previous == L"*"; if (boundary) { const auto body = body_after(closing[n + 1] + 1); if (body != SIZE_MAX && (value(body) == L"{" || (class_depth && !function_depth))) { add(n); if (value(body) == L"{") body_braces.insert(body); } } }
        }
        if (result.size() == 4096 && limited) break;
    }
    if (cancel) return false; std::sort(result.begin(),result.end(),[](const auto& a,const auto& b) { return a.line < b.line; }); *truncated = limited; *output = std::move(result); return true;
}
}

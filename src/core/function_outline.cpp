#include "function_outline.h"
#include "text_codec.h"
#include <cwctype>
#include <set>
namespace simple_editor::core {
namespace {
struct Token { std::wstring value; std::size_t begin,line; };
bool Word(wchar_t c) { return c == L'_' || iswalnum(c); }
bool Function(const std::vector<Token>& tokens,const std::wstring& source,MarkdownHeading* heading) {
    if (tokens.empty() || tokens.size() > 512) return false;
    static const std::set<std::wstring> forbidden{L"if",L"for",L"while",L"switch",L"catch",L"namespace",L"class",L"struct",L"union",L"enum",L"return",L"sizeof",L"decltype",L"alignof",L"static_assert",L"operator",L"requires"};
    std::size_t begin = 0; if (tokens.size() > 2 && (tokens[0].value == L"public" || tokens[0].value == L"private" || tokens[0].value == L"protected") && tokens[1].value == L":") begin = 2;
    if (begin == tokens.size() || forbidden.count(tokens[begin].value)) return false;
    for (std::size_t i = begin; i < tokens.size(); ++i) {
        if (tokens[i].value == L"=" || tokens[i].value == L"[" || tokens[i].value == L"#literal") return false;
        if (tokens[i].value != L"(") continue;
        if (i == begin || tokens[i - 1].value.empty() || (!iswalpha(tokens[i - 1].value[0]) && tokens[i - 1].value[0] != L'_') || forbidden.count(tokens[i - 1].value)) return false;
        unsigned depth = 1; std::size_t end = i + 1;
        for (; end < tokens.size() && depth; ++end) { if (tokens[end].value == L"(") ++depth; else if (tokens[end].value == L")") --depth; }
        if (depth) return false;
        for (std::size_t n = end; n < tokens.size(); ++n) if (tokens[n].value == L":" || tokens[n].value == L"=" || tokens[n].value == L"#literal" || tokens[n].value == L"requires") return false;
        const auto first = tokens[begin].begin,last = tokens.back().begin + tokens.back().value.size(); heading->title = source.substr(first,(std::min<std::size_t>)(160,last - first));
        for (auto& c : heading->title) if (c == L'\r' || c == L'\n' || c == L'\t') c = L' ';
        if (!heading->title.empty() && heading->title.back() >= 0xD800 && heading->title.back() <= 0xDBFF) heading->title.pop_back(); heading->line = tokens[i - 1].line; heading->level = 1; return true;
    } return false;
}
}
bool CppFunctionOutline(const std::wstring& text,const std::atomic_bool& cancel,std::vector<MarkdownHeading>* output,bool* truncated) {
    if (!output || !truncated || text.size() > 32 * 1024 * 1024 || !ValidateText(text) || cancel) return false;
    std::vector<MarkdownHeading> result; std::vector<Token> header; std::size_t i = 0,line = 1,tokens = 0; unsigned braces = 0,function_depth = 0; bool start = true,overflow = false; *truncated = false;
    const auto advance = [&]() { if (text[i] == L'\r') { if (i + 1 < text.size() && text[i + 1] == L'\n') ++i; ++line; start = true; } else if (text[i] == L'\n') { ++line; start = true; } ++i; };
    const auto emit = [&](std::wstring value,std::size_t at,std::size_t row) { if (header.size() < 512) header.push_back({std::move(value),at,row}); else overflow = true; };
    while (i < text.size()) {
        if (cancel) return false; const auto c = text[i]; if (iswspace(c)) { advance(); continue; }
        if (c == L'#' && start) { do { bool continuation = false; while (i < text.size() && text[i] != L'\r' && text[i] != L'\n') { continuation = text[i] == L'\\'; advance(); } if (i < text.size()) advance(); if (!continuation) break; } while (i < text.size()); header.clear(); overflow = false; continue; }
        if (c == L'/' && i + 1 < text.size() && text[i + 1] == L'/') { while (i < text.size() && text[i] != L'\r' && text[i] != L'\n') advance(); continue; }
        if (c == L'/' && i + 1 < text.size() && text[i + 1] == L'*') { i += 2; while (i < text.size() && !(text[i] == L'*' && i + 1 < text.size() && text[i + 1] == L'/')) advance(); if (i < text.size()) i += 2; continue; }
        const auto at = i,row = line; start = false;
        if (c == L'"' || c == L'\'') {
            bool raw = c == L'"' && i && text[i - 1] == L'R'; std::size_t end = std::wstring::npos;
            if (raw) { const auto open = text.find(L'(',i + 1); if (open != std::wstring::npos && open - i <= 17) { const auto close = L")" + text.substr(i + 1,open - i - 1) + L"\""; const auto found = text.find(close,open + 1); if (found != std::wstring::npos) end = found + close.size(); } }
            if (end != std::wstring::npos) while (i < end) advance();
            else { advance(); while (i < text.size()) { if (text[i] == L'\\') { advance(); if (i < text.size()) advance(); } else { const auto next = text[i]; advance(); if (next == c) break; } } }
            emit(L"#literal",at,row); continue;
        }
        if (Word(c)) { advance(); while (i < text.size() && Word(text[i])) advance(); emit(text.substr(at,(std::min<std::size_t>)(160,i - at)),at,row); }
        else if (c == L'{' || c == L'}' || c == L';') {
            if (c == L'{') { MarkdownHeading heading; const bool found = !function_depth && !overflow && Function(header,text,&heading); if (found) { if (result.size() == 4096) { *truncated = true; break; } result.push_back(std::move(heading)); } if (braces == 1024) { *truncated = true; break; } ++braces; if (found) function_depth = braces; }
            else if (c == L'}') { if (braces == function_depth) function_depth = 0; if (braces) --braces; }
            header.clear(); overflow = false; advance();
        } else { emit(std::wstring(1,c),at,row); advance(); }
        if (++tokens > 1000000) { *truncated = true; break; }
    }
    if (cancel) return false; *output = std::move(result); return true;
}
}

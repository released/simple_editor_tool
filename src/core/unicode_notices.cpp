#include "unicode_notices.h"
#include "search.h"
#include <sstream>
namespace simple_editor::core {
namespace {
struct Entry { std::uint32_t point; const wchar_t* name; };
constexpr Entry entries[] = {{0xA0,L"NO-BREAK SPACE"},{0x61C,L"ARABIC LETTER MARK"},{0x200B,L"ZERO WIDTH SPACE"},{0x200E,L"LEFT-TO-RIGHT MARK"},{0x200F,L"RIGHT-TO-LEFT MARK"},{0x2028,L"LINE SEPARATOR"},{0x2029,L"PARAGRAPH SEPARATOR"},{0x202A,L"LEFT-TO-RIGHT EMBEDDING"},{0x202B,L"RIGHT-TO-LEFT EMBEDDING"},{0x202C,L"POP DIRECTIONAL FORMATTING"},{0x202D,L"LEFT-TO-RIGHT OVERRIDE"},{0x202E,L"RIGHT-TO-LEFT OVERRIDE"},{0x2060,L"WORD JOINER"},{0x2066,L"LEFT-TO-RIGHT ISOLATE"},{0x2067,L"RIGHT-TO-LEFT ISOLATE"},{0x2068,L"FIRST STRONG ISOLATE"},{0x2069,L"POP DIRECTIONAL ISOLATE"},{0xFEFF,L"EMBEDDED BOM / ZERO WIDTH NO-BREAK SPACE"}};
}
const wchar_t* UnicodeNoticeName(std::uint32_t point) { for (const auto& entry : entries) if (entry.point == point) return entry.name; return nullptr; }
std::vector<std::uint32_t> UnicodeNoticePoints() { std::vector<std::uint32_t> result; for (const auto& entry : entries) result.push_back(entry.point); return result; }
bool ParseUnicodeAllowlist(const std::wstring& text,std::set<std::uint32_t>* output,std::wstring* error) {
    if (text.size() > 4096) { if (error) *error = L"Allowlist is limited to 4096 characters and 128 code points."; return false; }
    auto value = text; for (auto& c : value) if (c == L',' || c == L';') c = L' '; std::wistringstream input(value); std::wstring token; std::set<std::uint32_t> result;
    while (input >> token) { if (token.rfind(L"U+",0) == 0 || token.rfind(L"u+",0) == 0) token.erase(0,2); std::uint64_t point = 0; if (!ParseAddress(token,&point) || point > 0x10FFFF || (point >= 0xD800 && point <= 0xDFFF) || result.size() == 128) { if (error) *error = L"Use Unicode scalar hexadecimal values, e.g. U+00A0, U+200B. Maximum 128 entries."; return false; } result.insert(static_cast<std::uint32_t>(point)); }
    *output = std::move(result); return true;
}
bool ScanUnicodeNotices(const std::string& input,const std::set<std::uint32_t>& allowed,const SaveControl& control,UnicodeNotices* output,std::wstring* error) {
    if (input.size() > 32 * 1024 * 1024 || (!input.empty() && !MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input.data(),static_cast<int>(input.size()),nullptr,0))) { if (error) *error = L"Unicode notices accept valid UTF-8 snapshots up to 32 MiB."; return false; }
    UnicodeNotices result; std::size_t line = 1,column = 1; bool previous_cr = false;
    std::size_t next_check = 0;
    for (std::size_t at = 0; at < input.size();) {
        if (at >= next_check) { next_check = at + 4096; if (control.cancelled && *control.cancelled) { if (error) *error = L"Unicode scan cancelled."; return false; } }
        const auto start = at; const auto first = static_cast<unsigned char>(input[at++]); const unsigned size = first < 0x80 ? 1u : first < 0xE0 ? 2u : first < 0xF0 ? 3u : 4u; std::uint32_t point = size == 1 ? first : first & ((1u << (7 - size)) - 1);
        for (unsigned i = 1; i < size; ++i) point = (point << 6) | (static_cast<unsigned char>(input[at++]) & 63);
        if (const auto* name = UnicodeNoticeName(point)) if (!allowed.count(point) && (point != 0xFEFF || start)) { ++result.total; if (result.rows.size() < 100000) result.rows.push_back({start,size,line,column,point,name}); }
        if (point == L'\r' || point == L'\n') { if (point != L'\n' || !previous_cr) ++line; column = 1; } else ++column; previous_cr = point == L'\r';
    }
    if (control.cancelled && *control.cancelled) { if (error) *error = L"Unicode scan cancelled."; return false; } *output = std::move(result); return true;
}
}

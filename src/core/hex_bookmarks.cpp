#include "hex_bookmarks.h"
#include "text_codec.h"
#include <sstream>
namespace simple_editor::core {
std::wstring EncodeSetting(const std::wstring& value) { const wchar_t* digits = L"0123456789ABCDEF"; std::wstring result; result.reserve(value.size() * 4); for (auto ch : value) for (int shift : {12,8,4,0}) result += digits[(ch >> shift) & 15]; return result; }
bool DecodeSetting(const std::wstring& encoded,std::wstring* output,std::size_t maximum) {
    if (!output || encoded.size() % 4 || encoded.size() / 4 > maximum) return false; std::wstring value;
    for (std::size_t i = 0; i < encoded.size(); i += 4) { unsigned ch = 0; for (unsigned j = 0; j < 4; ++j) { const auto c = encoded[i + j]; const int digit = c >= L'0' && c <= L'9' ? c - L'0' : c >= L'A' && c <= L'F' ? c - L'A' + 10 : c >= L'a' && c <= L'f' ? c - L'a' + 10 : -1; if (digit < 0) return false; ch = ch * 16 + digit; } value += static_cast<wchar_t>(ch); }
    if (!ValidateText(value)) return false; *output = std::move(value); return true;
}
bool HexBookmarks::Add(const NamedHexRange& range,std::uint64_t size,std::wstring* error) {
    if (ranges.size() >= 256 || !range.length || range.offset > size || range.length > size - range.offset || range.name.empty() || range.name.size() > 128 || range.comment.size() > 512 || range.color > 0xFFFFFF || !ValidateText(range.name) || !ValidateText(range.comment)) { if (error) *error = L"Use a valid nonempty byte range, a name up to 128 characters and a comment up to 512 characters (256 bookmarks maximum)."; return false; } ranges.push_back(range); return true;
}
std::wstring HexBookmarks::Save(bool matches) const {
    if (ranges.empty()) return L"";
    std::wostringstream text; text << 1 << L' ' << (stale || !matches) << L' ' << source.volume << L' ' << source.identity << L' ' << source.size << L' ' << source.modified << L'\n';
    for (const auto& range : ranges) text << range.offset << L' ' << range.length << L' ' << range.color << L' ' << range.stale << L' ' << EncodeSetting(range.name) << L' ' << (range.comment.empty() ? L"-" : EncodeSetting(range.comment)) << L'\n'; return EncodeSetting(text.str());
}
bool HexBookmarks::Load(const std::wstring& encoded,std::uint64_t size,std::wstring* error) {
    if (encoded.empty()) { *this = {}; return true; } std::wstring value; if (!DecodeSetting(encoded,&value,180000)) { if (error) *error = L"Invalid bookmark settings."; return false; }
    std::wistringstream input(value); unsigned version = 0,invalid = 0; HexBookmarks result; if (!(input >> version >> invalid >> result.source.volume >> result.source.identity >> result.source.size >> result.source.modified) || version != 1 || invalid > 1) return false; result.source.exists = true; result.stale = invalid != 0;
    NamedHexRange range; std::wstring name,comment; while (input >> range.offset) { if (!(input >> range.length >> range.color >> invalid >> name >> comment) || invalid > 1 || !DecodeSetting(name,&range.name,128) || (comment != L"-" && !DecodeSetting(comment,&range.comment,512))) return false; range.stale = invalid != 0; if (comment == L"-") range.comment.clear(); if (!result.Add(range,(std::max)(size,result.source.size),error)) return false; if (range.offset > size || range.length > size - (std::min)(size,range.offset)) result.stale = true; } if (!input.eof()) return false; *this = std::move(result); return true;
}
bool HexBookmarks::Matches(const BinaryDocument& document) const { FileStamp current; return !stale && document.IsOpen() && InspectFile(document.Path(),&current,nullptr) && SameFileState(source,current) && document.ValidateSource(true); }
}

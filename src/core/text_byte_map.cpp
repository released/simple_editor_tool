#include "text_byte_map.h"
#include <algorithm>
namespace simple_editor::core {
bool MapTextByteRange(const std::wstring& raw,const TextCodec& codec,bool from_text,std::uint64_t begin,std::uint64_t end,const SaveControl& control,TextByteRange* output,std::wstring* error) {
    const auto fail = [&](const wchar_t* message) { if (error) *error = message; return false; };
    if (!output || begin > end || raw.size() > 32 * 1024 * 1024 || codec.encoding == TextEncoding::Auto || !ValidateText(raw,error)) return fail(L"Invalid selection, text or encoding for position mapping.");
    const bool utf16 = codec.encoding == TextEncoding::Utf16Le || codec.encoding == TextEncoding::Utf16Be,utf8 = codec.encoding == TextEncoding::Utf8 || (!utf16 && codec.code_page == CP_UTF8);
    std::vector<unsigned short> widths(65536,0); for (unsigned i = 0; i < 128; ++i) widths[i] = 1;
    std::uint64_t text = 0,bytes = codec.bom ? utf16 ? 2 : utf8 ? 3 : 0 : 0; bool found_begin = false,found_end = false; TextByteRange result;
    const auto boundary = [&]() { const auto position = from_text ? text : bytes; if (position == begin) { result.text_begin = text; result.byte_begin = bytes; found_begin = true; } if (position == end) { result.text_end = text; result.byte_end = bytes; found_end = true; } };
    boundary();
    std::size_t next_check = 0;
    for (std::size_t at = 0; at < raw.size();) {
        if (at >= next_check) { next_check = at + 4096; if (control.cancelled && *control.cancelled) return fail(L"Position mapping cancelled."); }
        const auto start = at; std::uint32_t point = raw[at++]; unsigned units = 1;
        if (point >= 0xD800 && point <= 0xDBFF) { point = 0x10000 + ((point - 0xD800) << 10) + (raw[at++] - 0xDC00); units = 2; }
        unsigned count = 0;
        if (utf16) count = units * 2;
        else if (utf8) count = point < 0x80 ? 1u : point < 0x800 ? 2u : point < 0x10000 ? 3u : 4u;
        else if (point <= 0xFFFF && widths[point]) count = widths[point];
        else { BOOL substituted = FALSE; const auto flags = codec.code_page == 54936 ? WC_ERR_INVALID_CHARS : WC_NO_BEST_FIT_CHARS; const int width = WideCharToMultiByte(codec.code_page,flags,raw.data() + start,units,nullptr,0,nullptr,codec.code_page == 54936 ? nullptr : &substituted); if (!width || substituted) return fail(L"The draft cannot be mapped losslessly in its selected encoding."); count = static_cast<unsigned>(width); if (point <= 0xFFFF) widths[point] = static_cast<unsigned short>(count); }
        const bool crlf_part = (point == L'\r' && at < raw.size() && raw[at] == L'\n') || (point == L'\n' && start && raw[start - 1] == L'\r');
        text += (point == L'\r' || point == L'\n') && !crlf_part ? 2u : units; bytes += count; boundary();
    }
    if (control.cancelled && *control.cancelled) return fail(L"Position mapping cancelled.");
    if (!found_begin || !found_end) return fail(L"The selection includes a BOM, partial encoded character, virtual newline half, or an out-of-range position. Choose complete characters/bytes.");
    *output = result; return true;
}
}

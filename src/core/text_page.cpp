#include "text_page.h"
#include <algorithm>
namespace simple_editor::core {
namespace {
bool Fail(std::wstring* error,const wchar_t* message) { if (error) *error = message; return false; }
bool Cancelled(const SaveControl& control) { return control.cancelled && *control.cancelled; }
unsigned Utf8Width(unsigned ch) { return ch < 0x80 ? 1 : ch >= 0xC2 && ch <= 0xDF ? 2 : ch >= 0xE0 && ch <= 0xEF ? 3 : ch >= 0xF0 && ch <= 0xF4 ? 4 : 0; }
}
bool ReadTextPage(const BinaryDocument& source,std::uint64_t offset,TextEncoding encoding,bool known_boundary,const SaveControl& control,TextPage* result,std::wstring* error,std::size_t budget) {
    if (!result || budget < 32 || budget > 1024 * 1024 || offset > source.Size()) return Fail(error,L"Invalid text page address or size.");
    if (Cancelled(control)) return Fail(error,L"Text page cancelled."); if (!source.ValidateSource(true,error)) return false;
    std::vector<std::uint8_t> head; if (!source.Read(0,static_cast<std::size_t>((std::min<std::uint64_t>)(source.Size(),4)),&head)) return Fail(error,L"Could not read the text header.");
    if (encoding == TextEncoding::Auto) encoding = head.size() >= 2 && head[0] == 0xFF && head[1] == 0xFE ? TextEncoding::Utf16Le : head.size() >= 2 && head[0] == 0xFE && head[1] == 0xFF ? TextEncoding::Utf16Be : TextEncoding::Utf8;
    const bool utf16 = encoding == TextEncoding::Utf16Le || encoding == TextEncoding::Utf16Be; const UINT cp = encoding == TextEncoding::Big5 ? 950 : encoding == TextEncoding::System ? GetACP() : CP_UTF8; const bool utf8 = !utf16 && cp == CP_UTF8;
    if (!utf16 && !utf8 && offset && !known_boundary) return Fail(error,L"Legacy code pages require sequential page boundaries; use Previous / Next or return to byte zero.");
    if (utf16 && offset % 2) --offset;
    TextPage page; page.total = source.Size(); page.begin = offset; std::vector<std::uint8_t> bytes;
    if (!source.Read(offset,static_cast<std::size_t>((std::min<std::uint64_t>)(page.total - offset,budget + 4)),&bytes)) return Fail(error,L"Could not read the text page.");
    std::size_t skip = 0;
    if (offset && !known_boundary && utf8) { while (skip < bytes.size() && skip < 3 && (bytes[skip] & 0xC0) == 0x80) ++skip; }
    if (offset && !known_boundary && utf16 && bytes.size() >= 2) { const unsigned ch = encoding == TextEncoding::Utf16Le ? bytes[0] | bytes[1] << 8 : bytes[0] << 8 | bytes[1]; if (ch >= 0xDC00 && ch <= 0xDFFF) skip = 2; }
    page.begin += skip; bytes.erase(bytes.begin(),bytes.begin() + skip); std::size_t length = (std::min)(bytes.size(),budget);
    const bool more = page.begin + length < page.total;
    if (utf16) { length -= length % 2; if (more && length >= 2) { const unsigned ch = encoding == TextEncoding::Utf16Le ? bytes[length - 2] | bytes[length - 1] << 8 : bytes[length - 2] << 8 | bytes[length - 1]; if (ch >= 0xD800 && ch <= 0xDBFF) length -= 2; } }
    else if (utf8 && more && length) { std::size_t lead = length - 1; while (lead && length - lead <= 3 && (bytes[lead] & 0xC0) == 0x80) --lead; const auto width = Utf8Width(bytes[lead]); if (width && width > length - lead) length = lead; }
    else if (!utf8 && !utf16 && more) { CPINFO info{}; if (!GetCPInfo(cp,&info) || info.MaxCharSize > 2) return Fail(error,L"This system code page is not supported by the text pager."); for (std::size_t i = 0; i < length; ++i) if (IsDBCSLeadByteEx(cp,bytes[i]) && ++i == length) --length; }
    if (!more && utf16 && bytes.size() % 2) return Fail(error,L"Incomplete UTF-16 code unit at end of file.");
    bytes.resize(length); page.end = page.begin + length;
    // DecodeText recognizes a BOM only at the file start. A prefix prevents a
    // literal U+FEFF at a later page boundary from being treated as a header.
    if (page.begin) { if (utf16) { bytes.insert(bytes.begin(),encoding == TextEncoding::Utf16Le ? std::initializer_list<std::uint8_t>{0x20,0} : std::initializer_list<std::uint8_t>{0,0x20}); } else bytes.insert(bytes.begin(),0x20); }
    if (!DecodeText(bytes,encoding,&page.text,&page.codec,error)) return false; if (page.begin) page.text.erase(0,1);
    if (Cancelled(control)) return Fail(error,L"Text page cancelled."); if (!source.ValidateSource(true,error)) return false; *result = std::move(page); return true;
}
}

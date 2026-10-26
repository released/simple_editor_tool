#include "text_page.h"
#include "search.h"
#include <algorithm>
namespace simple_editor::core {
bool FindPagedText(const BinaryDocument& source,const TextCodec& codec,const std::wstring& query,std::uint64_t start,const SaveControl& control,std::uint64_t* found,std::uint64_t* after,std::wstring* error) {
    const auto fail = [&](const wchar_t* text) { if (error) *error = text; return false; };
    if (!found || !after || query.empty() || query.size() > 4096 || start > source.Size() || !ValidateText(query,error)) return fail(L"Enter 1-4096 valid Unicode characters for paged search.");
    const bool utf16 = codec.encoding == TextEncoding::Utf16Le || codec.encoding == TextEncoding::Utf16Be;
    if (!utf16 && codec.code_page != CP_UTF8) return fail(L"Paged text search currently supports UTF-8 and UTF-16. Use byte search for this code page.");
    if ((control.cancelled && *control.cancelled) || !source.ValidateSource(true,error)) return fail(L"Paged search cancelled or source changed.");
    std::vector<std::uint8_t> pattern,head; if (!EncodeText(query,codec,false,&pattern,error) || !source.Read(0,static_cast<std::size_t>((std::min<std::uint64_t>)(4,source.Size())),&head)) return false;
    std::uint64_t bom = 0; if (utf16 && head.size() >= 2 && ((codec.encoding == TextEncoding::Utf16Le && head[0] == 0xFF && head[1] == 0xFE) || (codec.encoding == TextEncoding::Utf16Be && head[0] == 0xFE && head[1] == 0xFF))) bom = 2;
    else if (!utf16 && head.size() >= 3 && head[0] == 0xEF && head[1] == 0xBB && head[2] == 0xBF) bom = 3;
    const std::atomic_bool idle{false}; const auto& cancel = control.cancelled ? *control.cancelled : idle; auto match = UINT64_MAX;
    if (!ScanMatches(source,pattern,(std::max)(start,bom),source.Size(),cancel,[&](std::uint64_t offset) { match = offset; return false; },error,utf16 ? 2 : 1) || cancel || !source.ValidateSource(true,error)) return false;
    *found = match; *after = match == UINT64_MAX ? source.Size() : match + pattern.size(); return true;
}
}

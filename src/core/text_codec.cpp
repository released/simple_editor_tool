#include "text_codec.h"
#include <algorithm>
#include <climits>

namespace simple_editor::core {
namespace {
bool ValidText(const std::wstring& text) {
    for (std::size_t i = 0; i < text.size(); ++i) {
        const wchar_t ch = text[i];
        if (ch == 0 || (ch >= 0xDC00 && ch <= 0xDFFF)) return false;
        if (ch >= 0xD800 && ch <= 0xDBFF) {
            if (++i >= text.size() || text[i] < 0xDC00 || text[i] > 0xDFFF) return false;
        }
    }
    return true;
}
bool Fail(std::wstring* error) {
    if (error) *error = L"The text is invalid in the selected encoding, contains NUL, or cannot be encoded without data loss. Use Hex mode or select another encoding.";
    return false;
}
}

bool ValidateText(const std::wstring& text,std::wstring* error) { return ValidText(text) || Fail(error); }

bool EncodeText(const std::wstring& text, const TextCodec& codec, bool include_bom,
                std::vector<std::uint8_t>* bytes, std::wstring* error) {
    bytes->clear();
    if (!ValidText(text) || text.size() > INT_MAX) return Fail(error);
    const bool utf16 = codec.encoding == TextEncoding::Utf16Le || codec.encoding == TextEncoding::Utf16Be;
    if (utf16) {
        const bool little = codec.encoding == TextEncoding::Utf16Le;
        if (include_bom && codec.bom) bytes->insert(bytes->end(), {static_cast<std::uint8_t>(little ? 0xFF : 0xFE), static_cast<std::uint8_t>(little ? 0xFE : 0xFF)});
        for (wchar_t ch : text) {
            bytes->push_back(static_cast<std::uint8_t>(little ? ch : ch >> 8));
            bytes->push_back(static_cast<std::uint8_t>(little ? ch >> 8 : ch));
        }
        return true;
    }
    const UINT cp = codec.code_page == CP_ACP ? GetACP() : codec.code_page;
    const bool utf8 = cp == CP_UTF8;
    BOOL substituted = FALSE;
    const DWORD flags = utf8 ? WC_ERR_INVALID_CHARS : WC_NO_BEST_FIT_CHARS;
    if (include_bom && codec.bom && utf8) bytes->insert(bytes->end(), {0xEF, 0xBB, 0xBF});
    if (text.empty()) return true;
    const int count = WideCharToMultiByte(cp, flags, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, utf8 ? nullptr : &substituted);
    if (!count || substituted) return Fail(error);
    const auto start = bytes->size(); bytes->resize(start + count);
    if (!WideCharToMultiByte(cp, flags, text.data(), static_cast<int>(text.size()), reinterpret_cast<char*>(bytes->data() + start), count, nullptr, utf8 ? nullptr : &substituted) || substituted) return Fail(error);
    return true;
}

bool DecodeText(const std::vector<std::uint8_t>& bytes, TextEncoding requested,
                std::wstring* text, TextCodec* codec, std::wstring* error) {
    text->clear(); *codec = TextCodec();
    if (bytes.size() > INT_MAX) return Fail(error);
    const bool le_bom = bytes.size() >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE;
    const bool be_bom = bytes.size() >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF;
    const bool utf8_bom = bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF;
    codec->encoding = requested == TextEncoding::Auto ? (le_bom ? TextEncoding::Utf16Le : be_bom ? TextEncoding::Utf16Be : TextEncoding::Utf8) : requested;
    std::size_t start = 0;
    if (codec->encoding == TextEncoding::Utf16Le || codec->encoding == TextEncoding::Utf16Be) {
        const bool little = codec->encoding == TextEncoding::Utf16Le;
        codec->bom = little ? le_bom : be_bom; start = codec->bom ? 2 : 0;
        if ((bytes.size() - start) % 2) return Fail(error);
        for (auto i = start; i < bytes.size(); i += 2) text->push_back(static_cast<wchar_t>(little ? bytes[i] | bytes[i + 1] << 8 : bytes[i] << 8 | bytes[i + 1]));
    } else {
        codec->code_page = codec->encoding == TextEncoding::Big5 ? 950 : codec->encoding == TextEncoding::System ? GetACP() : CP_UTF8;
        codec->bom = utf8_bom && codec->code_page == CP_UTF8; start = codec->bom ? 3 : 0;
        const int length = static_cast<int>(bytes.size() - start);
        const char* input = length ? reinterpret_cast<const char*>(bytes.data() + start) : "";
        int count = length ? MultiByteToWideChar(codec->code_page, MB_ERR_INVALID_CHARS, input, length, nullptr, 0) : 0;
        if (!count && length && requested == TextEncoding::Auto && !utf8_bom) {
            codec->encoding = TextEncoding::System; codec->code_page = GetACP();
            count = MultiByteToWideChar(codec->code_page, MB_ERR_INVALID_CHARS, input, length, nullptr, 0);
        }
        if (!count && length) return Fail(error);
        text->resize(count);
        if (count) MultiByteToWideChar(codec->code_page, MB_ERR_INVALID_CHARS, input, length, text->data(), count);
    }
    if (!ValidText(*text)) return Fail(error);
    std::vector<std::uint8_t> roundtrip;
    if (!EncodeText(*text, *codec, true, &roundtrip, error) || roundtrip != bytes) return Fail(error);
    return true;
}
} // namespace simple_editor::core

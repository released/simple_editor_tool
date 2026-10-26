#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <cstdint>

namespace simple_editor::core {
enum class TextEncoding { Auto, Utf8, Utf16Le, Utf16Be, Big5, System };
struct TextCodec {
    TextEncoding encoding = TextEncoding::Utf8;
    UINT code_page = CP_UTF8;
    bool bom = false;
};
std::wstring DescribeTextCodec(const TextCodec&,const std::wstring& text);
bool ValidateText(const std::wstring& text,std::wstring* error = nullptr);
bool DecodeText(const std::vector<std::uint8_t>& bytes, TextEncoding requested,
                std::wstring* text, TextCodec* codec, std::wstring* error);
bool EncodeText(const std::wstring& text, const TextCodec& codec, bool include_bom,
                std::vector<std::uint8_t>* bytes, std::wstring* error);
} // namespace simple_editor::core

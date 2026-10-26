#include "data_conversion.h"
#include <wincrypt.h>
#include <algorithm>
namespace simple_editor::core {
namespace {
int Digit(unsigned char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; }
int Sextet(unsigned char c) { return c >= 'A' && c <= 'Z' ? c - 'A' : c >= 'a' && c <= 'z' ? c - 'a' + 26 : c >= '0' && c <= '9' ? c - '0' + 52 : c == '+' ? 62 : c == '/' ? 63 : -1; }
bool Space(unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
}
bool ConvertData(const std::string& input,DataFormat input_format,DataFormat output_format,const SaveControl& control,DataConversion* output,std::wstring* error) {
    const auto fail = [&](const wchar_t* text) { if (error) *error = text; return false; }; const auto cancelled = [&]() { return control.cancelled && *control.cancelled; };
    if (!output || input.size() > 32 * 1024 * 1024 || static_cast<unsigned>(input_format) > 2 || static_cast<unsigned>(output_format) > 2 || cancelled()) return fail(L"Conversion cancelled or input exceeds 32 MiB.");
    DataConversion result;
    if (input_format == DataFormat::Hex) {
        int high = -1; for (std::size_t i = 0; i < input.size(); ++i) { if (!(i & 4095) && cancelled()) return fail(L"Conversion cancelled."); const auto c = static_cast<unsigned char>(input[i]); if (Space(c)) continue; const auto digit = Digit(c); if (digit < 0) return fail(L"Hex input accepts hexadecimal byte pairs and whitespace only; no addresses or 0x prefixes."); if (high < 0) high = digit; else { result.bytes.push_back(static_cast<std::uint8_t>(high * 16 + digit)); high = -1; } } if (high >= 0) return fail(L"Hex input has an incomplete byte pair.");
    } else if (input_format == DataFormat::Base64) {
        std::string compact; compact.reserve(input.size()); for (std::size_t i = 0; i < input.size(); ++i) { if (!(i & 4095) && cancelled()) return fail(L"Conversion cancelled."); if (!Space(static_cast<unsigned char>(input[i]))) compact += input[i]; }
        if (compact.size() % 4) return fail(L"Standard Base64 requires complete groups and correct = padding.");
        std::size_t padding = 0; while (padding < compact.size() && compact[compact.size() - padding - 1] == '=') ++padding;
        if (padding > 2) return fail(L"Invalid Base64 padding.");
        for (std::size_t i = 0; i < compact.size() - padding; ++i) if (Sextet(static_cast<unsigned char>(compact[i])) < 0) return fail(L"Invalid Base64 character or misplaced padding.");
        if (padding && (Sextet(static_cast<unsigned char>(compact[compact.size() - padding - 1])) & (padding == 2 ? 15 : 3))) return fail(L"Base64 has nonzero unused padding bits.");
        DWORD size = 0; if (!compact.empty()) { if (!CryptStringToBinaryA(compact.data(),static_cast<DWORD>(compact.size()),CRYPT_STRING_BASE64 | CRYPT_STRING_STRICT,nullptr,&size,nullptr,nullptr) || size > 16 * 1024 * 1024) return fail(L"Invalid Base64 or decoded payload exceeds 16 MiB."); result.bytes.resize(size); if (!CryptStringToBinaryA(compact.data(),static_cast<DWORD>(compact.size()),CRYPT_STRING_BASE64 | CRYPT_STRING_STRICT,result.bytes.data(),&size,nullptr,nullptr)) return fail(L"Base64 decoding failed."); result.bytes.resize(size); }
    } else {
        if (input.size() > 16 * 1024 * 1024 || input.find('\0') != std::string::npos || (!input.empty() && !MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input.data(),static_cast<int>(input.size()),nullptr,0))) return fail(L"Text input must be valid UTF-8 without NUL, up to 16 MiB."); result.bytes.assign(input.begin(),input.end());
    }
    if (result.bytes.size() > 16 * 1024 * 1024 || cancelled()) return fail(L"Conversion cancelled or decoded payload exceeds 16 MiB.");
    if (output_format == DataFormat::Hex) { constexpr char hex[] = "0123456789ABCDEF"; result.output.reserve(result.bytes.size() * 3); for (std::size_t i = 0; i < result.bytes.size(); ++i) { if (!(i & 4095) && cancelled()) return fail(L"Conversion cancelled."); if (i) result.output += i % 16 ? ' ' : '\n'; result.output += hex[result.bytes[i] >> 4]; result.output += hex[result.bytes[i] & 15]; } }
    else if (output_format == DataFormat::Base64) { if (!result.bytes.empty()) { DWORD size = 0; if (!CryptBinaryToStringA(result.bytes.data(),static_cast<DWORD>(result.bytes.size()),CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,nullptr,&size)) return fail(L"Base64 output sizing failed."); result.output.resize(size); if (!CryptBinaryToStringA(result.bytes.data(),static_cast<DWORD>(result.bytes.size()),CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,result.output.data(),&size)) return fail(L"Base64 output failed."); result.output.resize(size); if (!result.output.empty() && !result.output.back()) result.output.pop_back(); } }
    else { result.output.assign(result.bytes.begin(),result.bytes.end()); if (result.output.find('\0') != std::string::npos || (!result.output.empty() && !MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,result.output.data(),static_cast<int>(result.output.size()),nullptr,0))) return fail(L"Decoded bytes are not NUL-free UTF-8. Choose Hex or Base64 output; no bytes applied."); }
    if (cancelled()) return fail(L"Conversion cancelled."); *output = std::move(result); return true;
}
}

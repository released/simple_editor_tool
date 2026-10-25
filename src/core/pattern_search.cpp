#include "pattern_search.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <locale>
#include <limits>
namespace simple_editor::core {
bool ParseBytePattern(const std::wstring& text,bool typed,BytePattern* pattern,std::wstring* error) {
    BytePattern result;
    if (typed) {
        const auto colon = text.find(L':'); if (colon == std::wstring::npos) { if (error) *error = L"Use type:value, for example u32le:1234, i16be:-2 or f32le:1.5."; return false; }
        auto type = text.substr(0,colon); for (auto& ch : type) ch = static_cast<wchar_t>(towlower(ch)); const bool big = type.size() > 2 && type.substr(type.size() - 2) == L"be"; if (big || (type.size() > 2 && type.substr(type.size() - 2) == L"le")) type.resize(type.size() - 2);
        unsigned width = 0; bool floating = false, signed_value = false;
        if (type == L"u8" || type == L"i8") width = 1; else if (type == L"u16" || type == L"i16") width = 2; else if (type == L"u32" || type == L"i32" || type == L"f32") width = 4; else if (type == L"u64" || type == L"i64" || type == L"f64") width = 8;
        if (!width) { if (error) *error = L"Supported types: u/i8, u/i16, u/i32, u/i64, f32, f64, followed by le or be."; return false; } floating = type[0] == L'f'; signed_value = type[0] == L'i'; std::uint64_t value = 0;
        auto number = text.substr(colon + 1); const auto first = number.find_first_not_of(L" \t"), last = number.find_last_not_of(L" \t"); if (first == std::wstring::npos) return false; number = number.substr(first,last - first + 1);
        try { std::size_t consumed = 0;
            if (floating) { std::wistringstream input(number); input.imbue(std::locale::classic()); long double number_value = 0; if (!(input >> number_value) || (input >> std::ws && !input.eof()) || !std::isfinite(number_value)) throw std::out_of_range("float"); if (width == 4) { if (std::abs(number_value) > (std::numeric_limits<float>::max)()) throw std::out_of_range("float"); const float converted = static_cast<float>(number_value); std::uint32_t bits; memcpy(&bits,&converted,4); value = bits; } else { const double converted = static_cast<double>(number_value); if (!std::isfinite(converted)) throw std::out_of_range("double"); memcpy(&value,&converted,8); } }
            else { const int base = number.find(L"0x") == 0 || number.find(L"0X") == 0 || number.find(L"-0x") == 0 || number.find(L"-0X") == 0 ? 16 : 10;
                if (signed_value) { const auto parsed = std::stoll(number,&consumed,base); if (width < 8 && (parsed < -(1LL << (width * 8 - 1)) || parsed > (1LL << (width * 8 - 1)) - 1)) throw std::out_of_range("signed"); memcpy(&value,&parsed,8); }
                else { if (number[0] == L'-') throw std::out_of_range("unsigned"); value = std::stoull(number,&consumed,base); if (width < 8 && value >= (1ULL << (width * 8))) throw std::out_of_range("unsigned"); }
                if (consumed != number.size()) throw std::invalid_argument("trailing");
            }
        } catch (...) { if (error) *error = L"Numeric value is invalid or outside the chosen type."; return false; }
        for (unsigned i = 0; i < width; ++i) result.bytes.push_back(static_cast<std::uint8_t>(value >> ((big ? width - i - 1 : i) * 8))); result.masks.assign(width,255);
    } else {
        int high = -1, high_mask = 0;
        for (auto ch : text) { if (iswspace(ch)) continue; ch = static_cast<wchar_t>(towupper(ch)); const int digit = ch == L'?' ? 0 : ch >= L'0' && ch <= L'9' ? ch - L'0' : ch >= L'A' && ch <= L'F' ? ch - L'A' + 10 : -1; if (digit < 0) { if (error) *error = L"Use Hex pairs with ? for a wildcard nibble: AA ?F ??."; return false; } const int mask = ch == L'?' ? 0 : 15;
            if (high < 0) { high = digit; high_mask = mask; } else { result.bytes.push_back(static_cast<std::uint8_t>(high * 16 + digit)); result.masks.push_back(static_cast<std::uint8_t>(high_mask * 16 + mask)); high = -1; }
        }
        if (high >= 0) { if (error) *error = L"Use two nibbles per byte."; return false; }
    }
    if (result.bytes.empty() || result.bytes.size() > 65536) { if (error) *error = L"Patterns support 1-65,536 bytes."; return false; } *pattern = std::move(result); return true;
}
bool ScanBytePattern(const BinaryDocument& document,const BytePattern& pattern,std::uint64_t begin,std::uint64_t end,const std::atomic_bool& cancel,const std::function<bool(std::uint64_t)>& match,const std::function<void(unsigned)>& progress,std::wstring* error) {
    if (begin > end || end > document.Size() || pattern.bytes.empty() || pattern.bytes.size() != pattern.masks.size() || pattern.bytes.size() > 65536 || !document.ValidateSource(true,error)) { if (error && error->empty()) *error = L"Invalid byte search range."; return false; }
    if (pattern.bytes.size() > end - begin) return true;
    std::vector<std::size_t> tests; for (std::size_t i = 0; i < pattern.masks.size(); ++i) if (pattern.masks[i]) tests.push_back(i);
    std::stable_sort(tests.begin(),tests.end(),[&](std::size_t a,std::size_t b) { return pattern.masks[a] > pattern.masks[b]; });
    const auto last = end - pattern.bytes.size();
    for (auto offset = begin; offset <= last && !cancel;) {
        const auto count = (std::min<std::uint64_t>)(1024 * 1024,last - offset + 1); std::vector<std::uint8_t> data; const auto needed = static_cast<std::size_t>(count + pattern.bytes.size() - 1);
        if (!document.Read(offset,needed,&data) || data.size() != needed) { if (error) *error = L"Could not read byte search input."; return false; }
        for (std::size_t i = 0; i < count; ++i) { if ((i & 4095) == 0 && cancel) return true; bool found = true; for (auto test : tests) if ((data[i + test] & pattern.masks[test]) != (pattern.bytes[test] & pattern.masks[test])) { found = false; break; } if (found && !match(offset + i)) return document.ValidateSource(true,error); }
        offset += count; if (progress) progress(static_cast<unsigned>(static_cast<long double>(offset - begin) * 100 / (std::max<std::uint64_t>)(1,end - begin)));
    }
    return document.ValidateSource(true,error);
}
}

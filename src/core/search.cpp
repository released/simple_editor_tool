#include "search.h"
#include <algorithm>
#include <cwctype>
#include <limits>

namespace simple_editor::core {

bool ParseAddress(const std::wstring& input, std::uint64_t* address) {
    auto begin = input.find_first_not_of(L" \t\r\n");
    if (!address || begin == std::wstring::npos) return false;
    const auto end = input.find_last_not_of(L" \t\r\n") + 1;
    unsigned base = 16;
    if (input.compare(begin, 2, L"d:") == 0 || input.compare(begin, 2, L"D:") == 0) { base = 10; begin += 2; }
    else if (input.compare(begin, 2, L"0x") == 0 || input.compare(begin, 2, L"0X") == 0) begin += 2;
    if (begin == end) return false;
    std::uint64_t result = 0;
    for (; begin < end; ++begin) {
        const wchar_t ch = towupper(input[begin]);
        const unsigned digit = ch >= L'0' && ch <= L'9' ? ch - L'0' : ch >= L'A' && ch <= L'F' ? ch - L'A' + 10 : 99;
        if (digit >= base || result > (UINT64_MAX - digit) / base) return false;
        result = result * base + digit;
    }
    *address = result; return true;
}

bool ParseHexBytes(const std::wstring& input, std::vector<std::uint8_t>* bytes) {
    bytes->clear(); int high = -1;
    for (wchar_t ch : input) {
        if (iswspace(ch)) continue;
        ch = towupper(ch);
        const int digit = ch >= L'0' && ch <= L'9' ? ch - L'0' : ch >= L'A' && ch <= L'F' ? ch - L'A' + 10 : -1;
        if (digit < 0) return false;
        if (high < 0) high = digit;
        else { bytes->push_back(static_cast<std::uint8_t>(high * 16 + digit)); high = -1; }
    }
    return high < 0;
}

bool ScanMatches(const BinaryDocument& document, const std::vector<std::uint8_t>& pattern,
                 std::uint64_t start, std::uint64_t end, const std::atomic_bool& cancel,
                 const std::function<bool(std::uint64_t)>& match, std::wstring* error,
                 std::size_t alignment) {
    if (pattern.empty() || !alignment || start > end || end > document.Size()) {
        if (error) *error = L"Invalid search range or empty search pattern.";
        return false;
    }
    std::vector<std::size_t> prefix(pattern.size(), 0);
    for (std::size_t i = 1, j = 0; i < pattern.size(); ++i) {
        while (j && pattern[i] != pattern[j]) j = prefix[j - 1];
        if (pattern[i] == pattern[j]) ++j;
        prefix[i] = j;
    }
    constexpr std::size_t chunk = 1024 * 1024;
    std::vector<std::uint8_t> bytes; std::size_t matched = 0;
    for (std::uint64_t at = start; at < end && !cancel; ) {
        const auto wanted = static_cast<std::size_t>((std::min<std::uint64_t>)(chunk, end - at));
        if (!document.Read(at, wanted, &bytes) || bytes.size() != wanted) {
            if (error) *error = L"Could not read the file while searching.";
            return false;
        }
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            if ((i & 4095) == 0 && cancel) return true;
            while (matched && bytes[i] != pattern[matched]) matched = prefix[matched - 1];
            if (bytes[i] == pattern[matched]) ++matched;
            if (matched == pattern.size()) {
                const auto offset = at + i + 1 - pattern.size();
                if (offset % alignment) { matched = prefix[matched - 1]; continue; }
                if (!match(offset)) return true;
                matched = prefix[matched - 1];
            }
        }
        at += bytes.size();
    }
    return true;
}
} // namespace simple_editor::core

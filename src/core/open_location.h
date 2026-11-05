#pragma once
#include <cstdint>
#include <limits>
#include <string>
#include <vector>
#include <utility>

namespace simple_editor::core {
struct OpenLocation { std::wstring path; std::uint64_t line = 0; };

inline bool ParseOpenLine(const std::wstring& value, std::uint64_t* line) {
    if (value.empty() || value.size() > 20) return false;
    std::uint64_t number = 0;
    for (const auto c : value) {
        if (c < L'0' || c > L'9') return false;
        const auto digit = static_cast<unsigned>(c - L'0');
        if (number > ((std::numeric_limits<std::uint64_t>::max)() - digit) / 10) return false;
        number = number * 10 + digit;
    }
    if (!number) return false;
    *line = number; return true;
}

inline bool ParseOpenLocations(const std::vector<std::wstring>& arguments, std::vector<OpenLocation>* locations) {
    std::vector<OpenLocation> result;
    if (!arguments.empty() && arguments.front() == L"--open") {
        if ((arguments.size() != 2 && arguments.size() != 4) || arguments[1].empty()) return false;
        OpenLocation location{arguments[1],0};
        if (arguments.size() == 4 && (arguments[2] != L"--line" || !ParseOpenLine(arguments[3],&location.line))) return false;
        result.push_back(std::move(location));
    } else {
        for (const auto& path : arguments) {
            if (path.empty() || path == L"--line" || path == L"--open") return false;
            result.push_back({path,0});
        }
    }
    for (const auto& location : result) if (location.path.size() > 32767 || location.path.find(L'\0') != std::wstring::npos) return false;
    *locations = std::move(result); return true;
}

// Versioned local IPC uses path/decimal-line pairs and a final empty field.
inline bool DecodeOpenLocations(const wchar_t* text, std::size_t length, std::vector<OpenLocation>* locations) {
    if (!text || length < 4 || text[length - 1] || text[length - 2]) return false;
    std::vector<std::wstring> fields;
    std::size_t start = 0;
    for (std::size_t index = 0; index + 1 < length; ++index) if (!text[index]) {
        if (index == start) return false;
        fields.emplace_back(text + start,index - start); start = index + 1;
    }
    if (fields.empty() || fields.size() % 2) return false;
    std::vector<OpenLocation> result;
    for (std::size_t index = 0; index < fields.size(); index += 2) {
        OpenLocation location{fields[index],0};
        if (location.path.size() > 32767 || (fields[index + 1] != L"0" && !ParseOpenLine(fields[index + 1],&location.line))) return false;
        result.push_back(std::move(location));
    }
    *locations = std::move(result); return true;
}
}

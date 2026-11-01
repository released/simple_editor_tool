#pragma once
#include <filesystem>
#include <string>
namespace simple_editor::core {
inline bool PreferHexFileView(const std::wstring& path) {
    const auto extension = std::filesystem::path(path).extension().wstring();
    return extension.size() == 4 && extension[0] == L'.' &&
        (extension[1] == L'h' || extension[1] == L'H') &&
        (extension[2] == L'e' || extension[2] == L'E') &&
        (extension[3] == L'x' || extension[3] == L'X');
}
}

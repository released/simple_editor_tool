#pragma once
#include "binary_document.h"
#include <filesystem>

namespace simple_editor::core {
struct OrdinalPathLess {
    bool operator()(const std::wstring& a, const std::wstring& b) const { return CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE) == CSTR_LESS_THAN; }
};
struct FileStamp {
    bool exists = false, directory = false;
    DWORD volume = 0;
    std::uint64_t identity = 0, size = 0, modified = 0;
};
bool CheckPlainPath(const std::filesystem::path& path, std::wstring* error);
bool InspectFile(const std::filesystem::path& path, FileStamp* stamp, std::wstring* error);
bool SameFileState(const FileStamp& a, const FileStamp& b);
bool WriteBytesSafely(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes,
                      std::wstring* error, const SaveControl& control = {}, const FileStamp* expected = nullptr, FileStamp* written = nullptr);
} // namespace simple_editor::core

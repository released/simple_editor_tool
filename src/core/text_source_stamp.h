#pragma once
#include "file_operations.h"
namespace simple_editor::core {
// Read-only observation follows editor open semantics. Sync/cleanup still reject reparse paths.
inline bool InspectTextSource(const std::wstring& path,FileStamp* stamp,std::wstring* error = nullptr) {
    *stamp = {};
    const auto handle = CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        const auto code = GetLastError(); if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND) return true;
        if (error) *error = L"Could not inspect the text source."; return false;
    }
    BY_HANDLE_FILE_INFORMATION info{}; const auto ok = GetFileInformationByHandle(handle,&info); CloseHandle(handle);
    if (!ok) { if (error) *error = L"Could not inspect the text source."; return false; }
    stamp->exists = true; stamp->directory = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    stamp->volume = info.dwVolumeSerialNumber; stamp->identity = (static_cast<std::uint64_t>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
    stamp->size = (static_cast<std::uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    stamp->modified = (static_cast<std::uint64_t>(info.ftLastWriteTime.dwHighDateTime) << 32) | info.ftLastWriteTime.dwLowDateTime; return true;
}
}

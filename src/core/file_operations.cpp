#include "file_operations.h"

namespace simple_editor::core {
bool CheckPlainPath(const std::filesystem::path& path, std::wstring* error) {
    std::error_code code; auto current = std::filesystem::absolute(path, code).lexically_normal();
    if (code || current.empty()) { if (error) *error = L"Invalid absolute path."; return false; }
    for (;;) {
        const DWORD attributes = GetFileAttributesW(current.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) { if (error) *error = L"Reparse points are not traversed: " + current.wstring(); return false; }
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            const DWORD failure = GetLastError();
            if (failure != ERROR_FILE_NOT_FOUND && failure != ERROR_PATH_NOT_FOUND) { if (error) *error = L"Cannot inspect path: " + current.wstring(); return false; }
        }
        const auto parent = current.parent_path(); if (parent.empty() || parent == current) break; current = parent;
    }
    return true;
}
bool InspectFile(const std::filesystem::path& path, FileStamp* stamp, std::wstring* error) {
    *stamp = {};
    if (!CheckPlainPath(path, error)) return false;
    HANDLE handle = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        const DWORD code = GetLastError();
        if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND) return true;
        if (error) *error = L"Cannot read file attributes: " + path.wstring(); return false;
    }
    BY_HANDLE_FILE_INFORMATION info{}; const BOOL ok = GetFileInformationByHandle(handle, &info); CloseHandle(handle);
    if (!ok || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) { if (error) *error = L"Path identity unavailable or changed to a reparse point: " + path.wstring(); return false; }
    stamp->exists = true; stamp->directory = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; stamp->volume = info.dwVolumeSerialNumber;
    stamp->identity = (static_cast<std::uint64_t>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
    stamp->size = (static_cast<std::uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    stamp->modified = (static_cast<std::uint64_t>(info.ftLastWriteTime.dwHighDateTime) << 32) | info.ftLastWriteTime.dwLowDateTime;
    return true;
}
bool SameFileState(const FileStamp& a, const FileStamp& b) {
    return a.exists == b.exists && (!a.exists || (a.directory == b.directory && a.volume == b.volume && a.identity == b.identity &&
        (a.directory || (a.size == b.size && a.modified == b.modified))));
}
bool WriteBytesSafely(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes,
                      std::wstring* error, const SaveControl& control, const FileStamp* expected, FileStamp* written) {
    FileStamp before;
    if (!InspectFile(path, &before, error)) return false;
    if (before.directory) { if (error) *error = L"Output is a directory, not a file."; return false; }
    if (expected && !SameFileState(before, *expected)) { if (error) *error = L"Output changed since the last save. Choose another output path."; return false; }
    wchar_t directory[MAX_PATH]{}, temporary[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH, directory) || !GetTempFileNameW(directory, L"sem", 0, temporary)) { if (error) *error = L"Could not create a private save source."; return false; }
    bool success = false;
    {
        BinaryDocument document;
        if (document.Open(before.exists ? path.wstring() : std::wstring(temporary), error) && document.Replace(0, document.Size(), bytes)) {
            FileStamp checked;
            if (InspectFile(path, &checked, error) && SameFileState(before, checked)) {
                std::atomic_bool stop{control.cancelled && control.cancelled->load()}; std::wstring commit_error; SaveControl guarded;
                guarded.cancelled = &stop; guarded.progress = [&](unsigned value) {
                    if (control.progress) control.progress(value);
                    if (control.cancelled && control.cancelled->load()) stop = true;
                    if (value == 99) { FileStamp now; if (!InspectFile(path, &now, &commit_error) || !SameFileState(now, before)) { stop = true; if (commit_error.empty()) commit_error = L"Output changed before replacement."; } }
                };
                success = document.Save(path.wstring(), error, guarded);
                if (!commit_error.empty() && !success && error) *error = commit_error;
                if (success && written) success = InspectFile(path, written, error) && document.ValidateSource(true, error);
            }
            else if (error) *error = L"Output changed while preparing the save. Nothing was overwritten.";
        }
    }
    DeleteFileW(temporary); if (!success && error && error->empty()) *error = L"Could not prepare or save output; existing file was retained."; return success;
}
} // namespace simple_editor::core

#include "temporary_documents.h"
#include <algorithm>
namespace simple_editor::core {
TemporaryDocuments::~TemporaryDocuments() {
    std::wstring ignored;
    if (root_.empty() || !CheckPlainPath(root_,&ignored)) return;
    for (const auto& file : files_) { FileStamp now; if (InspectFile(file.first,&now,&ignored) && SameFileState(now,file.second) && now.size == 0) DeleteFileW(file.first.c_str()); }
    RemoveDirectoryW(root_.c_str());
}
std::filesystem::path TemporaryDocuments::Create(std::wstring* error) {
    if (root_.empty()) {
        wchar_t temporary[MAX_PATH]{}; const auto length = GetTempPathW(MAX_PATH,temporary); if (!length || length >= MAX_PATH) { if (error) *error = L"The temporary directory is unavailable or too long."; return {}; }
        static std::atomic_uint64_t sequence{0}; root_ = std::filesystem::path(temporary) / (L"SimpleEditorTool-empty-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()) + L"-" + std::to_wstring(++sequence));
        if (!CreateDirectoryW(root_.c_str(),nullptr)) { root_.clear(); if (error) *error = L"Could not create an owned temporary document directory."; return {}; }
    }
    if (!CheckPlainPath(root_,error)) return {};
    wchar_t name[MAX_PATH]{}; if (!GetTempFileNameW(root_.c_str(),L"set",0,name)) { if (error) *error = L"Could not create an empty document backing file."; return {}; }
    FileStamp stamp; const std::filesystem::path path(name); if (!InspectFile(path,&stamp,error) || !stamp.exists || stamp.directory || stamp.size != 0) return {};
    files_.push_back({path,stamp}); return path;
}
bool TemporaryDocuments::Owns(const std::filesystem::path& path) const {
    std::error_code error; const auto normalized = std::filesystem::absolute(path,error).lexically_normal(); if (error) return false;
    return std::any_of(files_.begin(),files_.end(),[&](const auto& file) { return CompareStringOrdinal(file.first.c_str(),-1,normalized.c_str(),-1,TRUE) == CSTR_EQUAL; });
}
}

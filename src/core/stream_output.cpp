#include "stream_output.h"
#include <algorithm>

namespace simple_editor::core {
StreamOutput::~StreamOutput() { if (file_ != INVALID_HANDLE_VALUE) CloseHandle(file_); if (!temporary_.empty()) DeleteFileW(temporary_.c_str()); }
bool StreamOutput::Open(const std::filesystem::path& destination, std::wstring* error, bool require_new) {
    if (file_ != INVALID_HANDLE_VALUE || !temporary_.empty()) return false;
    if (!InspectFile(destination,&expected_,error) || expected_.directory) { if (error && error->empty()) *error = L"Choose a file output path."; return false; }
    if (require_new && expected_.exists) { if (error) *error = L"Output already exists; nothing was overwritten."; return false; }
    std::error_code code; destination_ = std::filesystem::absolute(destination,code).lexically_normal();
    if (code || !CheckPlainPath(destination_.parent_path(),error)) return false;
    for (unsigned attempt = 0; attempt < 100; ++attempt) {
        temporary_ = destination_.parent_path() / (L".set-output-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()) + L"-" + std::to_wstring(attempt) + L".tmp");
        file_ = CreateFileW(temporary_.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
        if (file_ != INVALID_HANDLE_VALUE) return true;
        if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS) break;
    }
    temporary_.clear(); if (error) *error = L"Could not create the output temporary file."; return false;
}
bool StreamOutput::Write(const void* bytes, std::size_t length, std::wstring* error) {
    auto* next = static_cast<const unsigned char*>(bytes);
    while (length) { const DWORD chunk = static_cast<DWORD>((std::min<std::size_t>)(length,1024 * 1024)); DWORD written = 0;
        if (file_ == INVALID_HANDLE_VALUE || !WriteFile(file_,next,chunk,&written,nullptr) || written != chunk) { if (error) *error = L"Could not write the output file."; return false; }
        next += chunk; length -= chunk;
    }
    return true;
}
bool StreamOutput::Text(const std::wstring& value, std::wstring* error) {
    if (value.empty()) return true;
    const int size = WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);
    if (!size) { if (error) *error = L"Output contains invalid Unicode."; return false; }
    std::string encoded(size,'\0'); WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),encoded.data(),size,nullptr,nullptr);
    return Write(encoded.data(),encoded.size(),error);
}
bool StreamOutput::Commit(const SaveControl& control, std::wstring* error) {
    if (file_ == INVALID_HANDLE_VALUE || !FlushFileBuffers(file_)) { if (error) *error = L"Could not flush the output file."; return false; }
    CloseHandle(file_); file_ = INVALID_HANDLE_VALUE;
    if (control.cancelled && *control.cancelled) { if (error) *error = L"Output cancelled before replacement."; return false; }
    BinaryDocument document; if (!document.Open(temporary_.wstring(),error)) return false;
    std::atomic_bool stopped{false}; std::wstring conflict; SaveControl guarded; guarded.cancelled = &stopped;
    guarded.progress = [&](unsigned percent) {
        if (control.progress) control.progress(percent);
        if (control.cancelled && *control.cancelled) stopped = true;
        if (percent == 99) { FileStamp now; if (!InspectFile(destination_,&now,&conflict) || !SameFileState(expected_,now)) { stopped = true; if (conflict.empty()) conflict = L"Output changed during processing; nothing was overwritten."; } }
    };
    const bool saved = document.Save(destination_.wstring(),error,guarded);
    if (!saved && !conflict.empty() && error) *error = conflict; return saved;
}
}

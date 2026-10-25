#pragma once
#include "file_operations.h"

namespace simple_editor::core {
class StreamOutput {
public:
    StreamOutput() = default;
    ~StreamOutput();
    StreamOutput(const StreamOutput&) = delete;
    StreamOutput& operator=(const StreamOutput&) = delete;
    bool Open(const std::filesystem::path& destination, std::wstring* error, bool require_new = false);
    bool Write(const void* bytes, std::size_t length, std::wstring* error);
    bool Text(const std::wstring& value, std::wstring* error);
    bool Commit(const SaveControl& control, std::wstring* error);
private:
    HANDLE file_ = INVALID_HANDLE_VALUE;
    std::filesystem::path temporary_, destination_;
    FileStamp expected_;
};
}

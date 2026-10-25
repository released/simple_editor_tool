#pragma once
#include "file_operations.h"
namespace simple_editor::core {
class TemporaryDocuments {
public:
    ~TemporaryDocuments();
    std::filesystem::path Create(std::wstring* error);
    bool Owns(const std::filesystem::path& path) const;
private:
    std::filesystem::path root_;
    std::vector<std::pair<std::filesystem::path,FileStamp>> files_;
};
}

#pragma once
#include "binary_document.h"
namespace simple_editor::core {
struct BytePattern { std::vector<std::uint8_t> bytes, masks; };
bool ParseBytePattern(const std::wstring& text,bool typed,BytePattern* pattern,std::wstring* error);
bool ScanBytePattern(const BinaryDocument& document,const BytePattern& pattern,std::uint64_t begin,std::uint64_t end,const std::atomic_bool& cancel,const std::function<bool(std::uint64_t)>& match,const std::function<void(unsigned)>& progress,std::wstring* error);
}

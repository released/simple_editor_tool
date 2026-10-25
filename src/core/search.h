#pragma once
#include "binary_document.h"
#include <atomic>
#include <functional>

namespace simple_editor::core {

bool ParseAddress(const std::wstring& input, std::uint64_t* address);
bool ParseHexBytes(const std::wstring& input, std::vector<std::uint8_t>* bytes);
bool ScanMatches(const BinaryDocument& document, const std::vector<std::uint8_t>& pattern,
                 std::uint64_t start, std::uint64_t end, const std::atomic_bool& cancel,
                 const std::function<bool(std::uint64_t)>& match, std::wstring* error,
                 std::size_t alignment = 1);

} // namespace simple_editor::core

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "binary_document.h"

namespace simple_editor::core {

constexpr std::uint64_t kMissingOffset = UINT64_MAX;

struct Difference {
    std::uint64_t left_offset;
    std::uint64_t right_offset;
    int left_byte;
    int right_byte;
    std::uint64_t left_anchor = kMissingOffset;
    std::uint64_t right_anchor = kMissingOffset;
};

using DifferenceBatch = std::vector<Difference>;
using BatchCallback = std::function<bool(const DifferenceBatch&)>;
using ProgressCallback = std::function<void(unsigned int)>;

bool CompareDocuments(const BinaryDocument& left, const BinaryDocument& right,
                      bool insertion_aware, const std::atomic_bool& cancelled,
                      const BatchCallback& batch_callback,
                      const ProgressCallback& progress_callback, std::wstring* error);

bool CompareFiles(const std::wstring& left_path, const std::wstring& right_path,
                  bool insertion_aware, const std::atomic_bool& cancelled,
                  const BatchCallback& batch_callback,
                  const ProgressCallback& progress_callback, std::wstring* error);

} // namespace simple_editor::core


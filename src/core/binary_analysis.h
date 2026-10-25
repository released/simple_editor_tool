#pragma once
#include "stream_output.h"
#include <array>

namespace simple_editor::core {
struct InspectorValue { std::wstring type, value; unsigned bytes = 0; bool valid = false; };
struct BinaryAnalysis {
    std::uint64_t bytes = 0;
    std::uint32_t crc32 = 0;
    std::wstring sha256;
    double entropy = 0;
    std::array<std::uint64_t,256> histogram{};
    std::vector<double> entropy_blocks;
};
bool InspectNumbers(const BinaryDocument& document, std::uint64_t offset, bool little_endian, std::vector<InspectorValue>* values, std::wstring* error);
bool AnalyzeBinary(const BinaryDocument& document, std::uint64_t offset, std::uint64_t length, const SaveControl& control, BinaryAnalysis* result, std::wstring* error);
enum class RangeTransform { Fill, Invert, Swap2, Swap4, Swap8, Reverse };
bool TransformBytes(BinaryDocument& document, std::uint64_t offset, std::uint64_t length, RangeTransform operation, const std::vector<std::uint8_t>& pattern, const SaveControl& control, std::wstring* error);
bool ExtractBytes(const BinaryDocument& document, std::uint64_t offset, std::uint64_t length, const std::filesystem::path& output, const SaveControl& control, std::wstring* error, bool source_array = false, bool require_new = false);
bool JoinBinaryFiles(const std::vector<std::filesystem::path>& inputs, const std::filesystem::path& output, const SaveControl& control, std::wstring* error);
bool SplitBinaryFile(const BinaryDocument& document, std::uint64_t part_bytes, const std::filesystem::path& output_prefix, const SaveControl& control, std::vector<std::filesystem::path>* completed, std::wstring* error);
}

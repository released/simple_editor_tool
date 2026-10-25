#pragma once
#include "stream_output.h"
namespace simple_editor::core {
enum class FirmwareFormat { IntelHex, SRecord };
struct FirmwareSegment { std::uint64_t address = 0; std::vector<std::uint8_t> bytes; };
struct FirmwareImage {
    std::vector<FirmwareSegment> segments;
    std::uint64_t data_bytes = 0, records = 0, entry = 0;
    bool has_entry = false;
    std::uint64_t Begin() const { return segments.empty() ? 0 : segments.front().address; }
    std::uint64_t End() const { return segments.empty() ? 0 : segments.back().address + segments.back().bytes.size(); }
};
bool ParseFirmware(const BinaryDocument& source, FirmwareFormat format, const SaveControl& control, FirmwareImage* image, std::wstring* error);
bool FirmwareToBinary(const BinaryDocument& source, const FirmwareImage& image, std::uint64_t start, std::uint64_t length, std::uint8_t fill, const std::filesystem::path& output, const SaveControl& control, std::wstring* error);
bool BinaryToFirmware(const BinaryDocument& source, std::uint64_t base, FirmwareFormat format, const std::filesystem::path& output, const SaveControl& control, std::wstring* error);
}

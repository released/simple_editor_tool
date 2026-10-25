#pragma once
#include "diff_engine.h"
#include "compare_region.h"
#include <filesystem>

namespace simple_editor::core {
enum class BinaryReportFormat { Csv, Tsv, Html };
struct BinaryReportStats { std::uint64_t changed = 0, added = 0, removed = 0, ranges = 0; };
bool ExportBinaryReport(const BinaryDocument& left, const BinaryDocument& right, bool insertion_aware,
    const std::filesystem::path& output, BinaryReportFormat format, const SaveControl& control,
    BinaryReportStats* stats, std::wstring* error);
bool ExportBinaryReportRegion(const BinaryDocument& left,const BinaryDocument& right,bool insertion_aware,const CompareRegion& region,
    const std::filesystem::path& output,BinaryReportFormat format,const SaveControl& control,BinaryReportStats* stats,std::wstring* error,bool require_new = false);
}

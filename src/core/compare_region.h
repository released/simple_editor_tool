#pragma once
#include "diff_engine.h"
namespace simple_editor::core {
struct AddressSpan { std::uint64_t begin = 0, length = 0; };
struct CompareRegion {
    std::uint64_t left_begin = 0, left_length = UINT64_MAX, right_begin = 0, right_length = UINT64_MAX;
    std::vector<AddressSpan> ignore_left, ignore_right;
};
bool ParseCompareRegionSpec(const std::wstring& text,CompareRegion* region,std::wstring* error);
bool CompareDocumentRegions(const BinaryDocument& left,const BinaryDocument& right,bool insertion_aware,const CompareRegion& region,const std::atomic_bool& cancelled,const BatchCallback& batch,const ProgressCallback& progress,std::wstring* error);
}

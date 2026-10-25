#pragma once
#include "diff_engine.h"
#include "stream_output.h"
#include <set>
namespace simple_editor::core {
struct BinaryPatch { std::uint64_t offset = 0, remove = 0, source = 0, length = 0; };
bool BuildBinaryPatches(const std::vector<Difference>& differences,const std::set<std::size_t>& selected,bool right_into_left,std::uint64_t left_size,std::uint64_t right_size,std::vector<BinaryPatch>* patches,std::wstring* error);
bool MergeBinaryRanges(const BinaryDocument& baseline,const BinaryDocument& donor,const std::vector<BinaryPatch>& patches,const std::filesystem::path& output,const SaveControl& control,std::wstring* error);
}

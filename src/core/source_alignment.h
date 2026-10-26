#pragma once
#include "line_merge.h"
#include <array>
namespace simple_editor::core {
struct SourceGap { std::size_t after_line = 0,lines = 0; };
using SourceGapPlan = std::array<std::vector<SourceGap>,3>;
bool BuildSourceGaps(const std::vector<TextMergeRow>& rows,const std::array<std::size_t,3>& sizes,bool three_way,SourceGapPlan* plan);
}

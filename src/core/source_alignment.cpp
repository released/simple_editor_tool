#include "source_alignment.h"
namespace simple_editor::core {
bool BuildSourceGaps(const std::vector<TextMergeRow>& rows,const std::array<std::size_t,3>& sizes,bool three_way,SourceGapPlan* output) {
    SourceGapPlan plan;
    for (unsigned side = three_way ? 0u : 1u; side < 3; ++side) {
        std::size_t next = 0,missing = 0;
        for (const auto& row : rows) {
            const int index = side == 0 ? row.base : side == 1 ? row.left : row.right;
            if (index < 0) { ++missing; continue; }
            if (static_cast<std::size_t>(index) != next || next >= sizes[side]) return false;
            if (missing) { plan[side].push_back({next,missing}); missing = 0; } ++next;
        }
        if (next != sizes[side]) return false;
        if (missing) plan[side].push_back({next,missing});
    }
    *output = std::move(plan); return true;
}
}

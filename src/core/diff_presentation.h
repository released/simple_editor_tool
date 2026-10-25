#pragma once
#include "line_merge.h"
namespace simple_editor::core {
struct InlineSpan { std::size_t begin = 0, length = 0; };
struct InlineDifference { std::vector<InlineSpan> left, right; };
struct DiffPresentation { std::vector<TextMergeRow> rows; std::vector<InlineDifference> characters; std::vector<std::wstring> hints; };
InlineDifference CompareLineCharacters(const std::wstring& left,const std::wstring& right,const LineOptions& options,std::uint64_t* cell_budget = nullptr);
bool BuildDiffPresentation(const TextMergeModel& model,const LineOptions& options,bool similar_lines,const std::atomic_bool& cancel,DiffPresentation* presentation);
}

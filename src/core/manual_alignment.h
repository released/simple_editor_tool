#pragma once
#include "line_merge.h"
namespace simple_editor::core {
using LineAnchors = std::vector<std::pair<std::size_t,std::size_t>>;
bool ParseLineAnchors(const std::wstring& text,LineAnchors* anchors,std::wstring* error);
bool BuildAnchoredComparison(const std::wstring& left,const std::wstring& right,const LineOptions& options,const LineAnchors& anchors,const std::atomic_bool& cancel,TextMergeModel* model,std::wstring* error);
}

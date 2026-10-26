#include "advanced_compare.h"
#include "feature_dialogs.h"
#include "../core/manual_alignment.h"
#include <algorithm>
using namespace simple_editor::core;
namespace {
std::wstring AnchorText(const LineAnchors& points) { std::wstring text; for (const auto& point : points) text += std::to_wstring(point.first + 1) + L"," + std::to_wstring(point.second + 1) + L"\r\n"; return text; }
}
bool CAdvancedCompareView::AddSelectedAnchor() {
    if (Busy() || stale_ || mode_ != 2 || pasted_inputs_ || !model_) { Status(L"Compare two files, select an original line in each source pane, then add an alignment point."); return false; }
    const auto line = [](CCodeEditor& editor) { return editor.OriginalLine(editor.Call(SCI_LINEFROMPOSITION,editor.Call(SCI_GETCURRENTPOS))); };
    const auto left = line(source_left_),right = line(source_right_);
    if (left < 0 || right < 0 || static_cast<std::size_t>(left) >= model_->left.size() || static_cast<std::size_t>(right) >= model_->right.size()) { Status(L"Choose original source lines, not a display gap."); return false; }
    LineAnchors points; std::wstring error; if (!ParseLineAnchors(manual_anchors_,&points,&error)) { Status(error); return false; }
    const std::pair<std::size_t,std::size_t> point{static_cast<std::size_t>(left),static_cast<std::size_t>(right)};
    if (std::find(points.begin(),points.end(),point) != points.end()) { Status(L"This alignment point already exists."); return false; }
    points.push_back(point); std::sort(points.begin(),points.end()); const auto text = AnchorText(points); LineAnchors checked;
    if (!ParseLineAnchors(text,&checked,&error)) { Status(error); return false; }
    manual_anchors_ = text; InvalidateInputs(); if (changed) changed(); return true;
}
void CAdvancedCompareView::ManageAnchors() {
    if (Busy() || mode_ != 2 || pasted_inputs_) { Status(L"Alignment points are available for two-way file comparison."); return; }
    LineAnchors points; std::wstring error; if (!ParseLineAnchors(manual_anchors_,&points,&error)) { Status(error); return; }
    std::vector<std::vector<std::wstring>> rows; for (const auto& point : points) rows.push_back({std::to_wstring(point.first + 1),std::to_wstring(point.second + 1)});
    CReviewList dialog(this,L"Alignment points",{L"Left line",L"Right line"},rows); dialog.accept_label = L"Apply points"; dialog.allow_empty = true;
    dialog.remove = [&](std::size_t index) { if (index >= points.size()) return false; points.erase(points.begin() + index); return true; };
    modal_ = true; const auto accepted = dialog.DoModal(); modal_ = false;
    if (accepted == IDOK) { const auto text = AnchorText(points); if (text != manual_anchors_) { manual_anchors_ = text; InvalidateInputs(); if (changed) changed(); } }
}
void CAdvancedCompareView::ShowAnchorMarkers() {
    for (auto* editor : {&source_left_,&source_right_}) { editor->Call(SCI_MARKERDELETEALL,27); editor->Call(SCI_SETMARGINMASKN,1,(1 << 20) | (1 << 27)); editor->Call(SCI_MARKERDEFINE,27,SC_MARK_SHORTARROW); editor->Call(SCI_MARKERSETBACK,27,RGB(210,110,230)); }
    if (stale_ || !model_ || mode_ != 2 || pasted_inputs_) return;
    LineAnchors points; std::wstring error; if (!ParseLineAnchors(manual_anchors_,&points,&error)) return;
    for (const auto& point : points) if (point.first < model_->left.size() && point.second < model_->right.size()) { source_left_.Call(SCI_MARKERADD,source_left_.NativeLine(point.first),27); source_right_.Call(SCI_MARKERADD,source_right_.NativeLine(point.second),27); }
}

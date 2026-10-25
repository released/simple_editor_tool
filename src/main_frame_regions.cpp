#include "main_frame.h"
#include "resource.h"
void CMainFrame::SynchronizeCompareScroll(bool from_left,std::uint64_t row) {
    auto& source = from_left ? compare_left_ : compare_right_; auto& target = from_left ? compare_right_ : compare_left_;
    const auto a = from_left ? compare_origin_left_ : compare_origin_right_, b = from_left ? compare_origin_right_ : compare_origin_left_; const auto address = row * source.BytesPerRow(); std::uint64_t mapped = 0;
    if (b >= a) { const auto delta = b - a; mapped = address > UINT64_MAX - delta ? UINT64_MAX : address + delta; } else mapped = address < a - b ? 0 : address - (a - b);
    target.SetTopRow(mapped / target.BytesPerRow());
}
void CMainFrame::OnComparisonRegion() {
    if (IsBusy() || AdvancedActive()) { status_.SetPaneText(0,L"Choose an idle binary comparison to configure ranges."); return; }
    CValueDialog dialog(this,L"Binary ranges and ignored spans",L"left-start,left-length,right-start,right-length;L:offset+length,...;R:offset+length,...\r\nHex by default; d: for decimal. Length all means to EOF. Ignored spans use absolute file offsets and filter the aligned differences. Example: 100,80,200,80;L:110+4;R:210+4. Reset: 0,all,0,all",compare_region_spec_.empty() ? L"0,all,0,all" : compare_region_spec_);
    simple_editor::core::CompareRegion region; dialog.validate = [&](const std::wstring& text,std::wstring* error) { return simple_editor::core::ParseCompareRegionSpec(text,&region,error); };
    path_dialog_ = true; const auto selected = dialog.DoModal(); path_dialog_ = false; if (selected != IDOK) return; compare_region_ = std::move(region); compare_region_spec_ = dialog.value;
    ClearComparison(); status_.SetPaneText(0,L"Binary ranges updated. Press Compare to compare the selected paths. Reports use the same range and ignore settings.");
}

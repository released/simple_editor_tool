#include "main_frame.h"
#include "resource.h"
#include "workspace_frame.h"
#include <algorithm>

void CMainFrame::InitializeZoom() {
    text_.selection_changed = [this]() { if (::GetFocus() == text_.GetSafeHwnd()) clone_active_ = false; if (workspace_) workspace_->TrackNavigation(this); if (active_tab_ == 1 && status_.GetSafeHwnd() && !IsBusy()) status_.SetPaneText(0,text_.Status().c_str()); };
    hex_.wheel_zoom = [this](short delta) { ZoomWheel(0, delta); };
    text_.wheel_zoom = [this](short delta) { ZoomWheel(1, delta); };
    compare_left_.wheel_zoom = compare_right_.wheel_zoom = differences_.wheel_zoom =
        [this](short delta) { ZoomWheel(2, delta); };
    auto* font = differences_.GetFont();
    if (font) font->GetLogFont(&comparison_base_font_);
    else ::GetObjectW(::GetStockObject(DEFAULT_GUI_FONT), sizeof(comparison_base_font_), &comparison_base_font_);
    UpdateZoomStatus();
}

void CMainFrame::ZoomWheel(int tab, short delta) {
    if (tab != active_tab_ || tab < 0 || tab > 2) return;
    wheel_remainder_[tab] += delta;
    const int steps = wheel_remainder_[tab] / WHEEL_DELTA;
    wheel_remainder_[tab] %= WHEEL_DELTA;
    if (steps) SetTabZoom(tab, zoom_percent_[tab] + steps * 10);
}

bool CMainFrame::SetTabZoom(int tab, int percent) {
    if (tab < 0 || tab > 2) return false;
    percent = (std::clamp)(percent, 50, 300);
    if (percent == zoom_percent_[tab]) { UpdateZoomStatus(); return true; }
    if (tab == 0) {
        if (!hex_.SetZoomPercent(percent)) return false;
    } else if (tab == 1) {
        CClientDC dc(&text_); CFont replacement;
        if (!replacement.CreatePointFont(MulDiv(content_font_tenths_,percent,100), text_face_.c_str(), &dc)) return false;
        const int first = text_.GetFirstVisibleLine();
        CFont previous; previous.Attach(text_font_.Detach()); text_font_.Attach(replacement.Detach());
        text_.SetFont(&text_font_, TRUE);
        if (text_clone_.GetSafeHwnd()) text_clone_.SetFont(&text_font_,TRUE);
        text_.LineScroll(first - text_.GetFirstVisibleLine());
    } else {
        LOGFONT description = comparison_base_font_;
        description.lfHeight = MulDiv(description.lfHeight, percent, 100);
        description.lfWidth = MulDiv(description.lfWidth, percent, 100);
        CFont replacement;
        if (!replacement.CreateFontIndirect(&description)) return false;
        if (!compare_left_.SetZoomPercent(percent)) return false;
        if (!compare_right_.SetZoomPercent(percent)) { compare_left_.SetZoomPercent(zoom_percent_[tab]); return false; }
        const int first = differences_.GetTopIndex();
        CFont previous; previous.Attach(comparison_font_.Detach()); comparison_font_.Attach(replacement.Detach());
        differences_.SetFont(&comparison_font_, TRUE);
        auto* header = differences_.GetHeaderCtrl();
        if (header && header->GetSafeHwnd()) header->SetFont(&comparison_font_, TRUE);
        CRect row;
        if (differences_.GetItemCount() && differences_.GetItemRect(0, row, LVIR_BOUNDS)) {
            differences_.Scroll(CSize(0, (first - differences_.GetTopIndex()) * row.Height()));
        }
    }
    zoom_percent_[tab] = percent;
    if (workspace_) workspace_->RememberView(this);
    if (tab == 2 && advanced_.GetSafeHwnd()) advanced_.SetZoom(percent);
    if (tab == 2) UpdateDifferenceColumns();
    UpdateZoomStatus(); return true;
}

void CMainFrame::UpdateDifferenceColumns() {
    CClientDC dc(&differences_);
    auto* font = differences_.GetFont(); auto* previous = font ? dc.SelectObject(font) : nullptr;
    const wchar_t* samples[] = {L"1000000", L"0xFFFFFFFFFFFFFFFF", L"FF", L"0xFFFFFFFFFFFFFFFF", L"FF"};
    for (int i = 0; i < 5; ++i) {
        wchar_t text[64]{}; LVCOLUMN column{}; column.mask = LVCF_TEXT; column.pszText = text; column.cchTextMax = 64;
        differences_.GetColumn(i, &column);
        const int width = (std::max)(dc.GetTextExtent(text).cx, dc.GetTextExtent(samples[i]).cx);
        differences_.SetColumnWidth(i, width + MulDiv(24, zoom_percent_[2], 100));
    }
    if (previous) dc.SelectObject(previous);
}

void CMainFrame::UpdateZoomStatus() {
    if (!status_.GetSafeHwnd()) return;
    CClientDC dc(&status_); auto* font = status_.GetFont();
    auto* previous = font ? dc.SelectObject(font) : nullptr;
    const int width = dc.GetTextExtent(L"Zoom: 300%").cx + 16;
    if (previous) dc.SelectObject(previous);
    status_.SetPaneInfo(1, ID_VIEW_ZOOM_STATUS, SBPS_NORMAL, width);
    CString text; text.Format(L"Zoom: %d%%", zoom_percent_[active_tab_]); status_.SetPaneText(1, text);
}

void CMainFrame::OnUpdateZoomCommand(CCmdUI* command) { command->Enable(TRUE); }

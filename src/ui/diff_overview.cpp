#include "diff_overview.h"
#include <algorithm>
BEGIN_MESSAGE_MAP(CDiffOverview,CWnd)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_SIZE()
END_MESSAGE_MAP()
BOOL CDiffOverview::Create(CWnd* parent,UINT id) { const auto name = AfxRegisterWndClass(CS_DBLCLKS,::LoadCursor(nullptr,IDC_HAND),reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),nullptr); return CWnd::Create(name,L"Difference overview",WS_CHILD | WS_VISIBLE,CRect(),parent,id); }
void CDiffOverview::SetBands(std::vector<unsigned char> bands) { bands_ = std::move(bands); pixels_.clear(); if (GetSafeHwnd()) Invalidate(); }
void CDiffOverview::SetViewport(std::size_t first,std::size_t count) { if (first_ == first && count_ == count) return; first_ = first; count_ = count; if (GetSafeHwnd()) Invalidate(); }
void CDiffOverview::OnSize(UINT type,int cx,int cy) { CWnd::OnSize(type,cx,cy); pixels_.clear(); }
void CDiffOverview::OnPaint() {
    CPaintDC dc(this); CRect rect; GetClientRect(rect); dc.FillSolidRect(rect,RGB(240,242,246)); if (rect.Height() <= 0 || bands_.empty()) return;
    if (pixels_.size() != static_cast<std::size_t>(rect.Height())) { pixels_.assign(rect.Height(),0); for (std::size_t i = 0; i < bands_.size(); ++i) { const auto y = (std::min<std::size_t>)(pixels_.size() - 1,i * pixels_.size() / bands_.size()); const auto band = bands_[i]; const auto priority = [](unsigned char value) { return value == 2 ? 4 : value == 3 ? 3 : value; }; const auto end = (std::min<std::size_t>)(pixels_.size(),(std::max<std::size_t>)(y + 1,(i + 1) * pixels_.size() / bands_.size())); for (auto pixel = y; pixel < end; ++pixel) if (priority(band) > priority(pixels_[pixel])) pixels_[pixel] = band; } }
    for (std::size_t y = 0; y < pixels_.size(); ++y) { const auto value = pixels_[y]; if (!value) continue; dc.FillSolidRect(2,static_cast<int>(y),(std::max)(0,rect.Width() - 4),1,value == 2 ? RGB(210,55,60) : value == 3 ? RGB(80,120,205) : RGB(220,165,45)); }
    const auto top = static_cast<int>((std::min)(first_,bands_.size()) * rect.Height() / bands_.size()), bottom = static_cast<int>((std::min)(first_ + count_,bands_.size()) * rect.Height() / bands_.size()); dc.Draw3dRect(0,top,rect.Width(),(std::max)(2,bottom - top),RGB(45,60,85),RGB(45,60,85));
}
void CDiffOverview::OnLButtonDown(UINT,CPoint point) { CRect rect; GetClientRect(rect); if (bands_.empty() || rect.Height() <= 0 || !navigate) return; const auto y = (std::clamp<LONG>)(point.y,0,rect.Height() - 1); navigate((std::min<std::size_t>)(bands_.size() - 1,static_cast<std::size_t>(y) * bands_.size() / rect.Height())); }

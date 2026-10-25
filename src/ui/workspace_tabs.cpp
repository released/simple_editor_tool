#include "workspace_tabs.h"
#include <cstdlib>
LRESULT CWorkspaceTabs::WindowProc(UINT message,WPARAM wparam,LPARAM lparam) {
    CPoint point(static_cast<short>(LOWORD(lparam)),static_cast<short>(HIWORD(lparam)));
    if (message == WM_MBUTTONUP) { const auto index = Hit(point); if (index >= 0 && close) close(index); return 0; }
    if (message == WM_CONTEXTMENU) {
        int index; if (point.x == -1 && point.y == -1) { index = GetCurSel(); CRect bounds; if (index >= 0) { GetItemRect(index,bounds); point = bounds.CenterPoint(); ClientToScreen(&point); } }
        else { auto local = point; ScreenToClient(&local); index = Hit(local); }
        if (index >= 0 && context) context(index,point); return 0;
    }
    if (message == WM_LBUTTONDOWN) { pressed_ = Hit(point); press_point_ = point; dragging_ = false; }
    if (message == WM_MOUSEMOVE && (wparam & MK_LBUTTON) && pressed_ >= 0 && !dragging_ && (std::abs(point.x - press_point_.x) >= GetSystemMetrics(SM_CXDRAG) || std::abs(point.y - press_point_.y) >= GetSystemMetrics(SM_CYDRAG))) { dragging_ = true; SetCapture(); }
    if (message == WM_LBUTTONUP) {
        const int from = pressed_,to = Hit(point); const bool dragged = dragging_; pressed_ = -1; dragging_ = false;
        if (GetCapture() == this) ReleaseCapture(); const auto result = CTabCtrl::WindowProc(message,wparam,lparam); if (dragged && from >= 0 && to >= 0 && from != to && reorder) reorder(from,to); return result;
    }
    if (message == WM_CAPTURECHANGED) { pressed_ = -1; dragging_ = false; }
    return CTabCtrl::WindowProc(message,wparam,lparam);
}

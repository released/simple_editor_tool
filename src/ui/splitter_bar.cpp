#include "splitter_bar.h"

BEGIN_MESSAGE_MAP(CSplitterBar, CWnd)
    ON_WM_PAINT()
    ON_WM_SETCURSOR()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_CANCELMODE()
END_MESSAGE_MAP()
void CSplitterBar::OnPaint() { CPaintDC dc(this); CRect r; GetClientRect(r); dc.FillSolidRect(r, GetSysColor(COLOR_3DSHADOW)); }
BOOL CSplitterBar::OnSetCursor(CWnd*, UINT, UINT) { ::SetCursor(::LoadCursor(nullptr, vertical ? IDC_SIZEWE : IDC_SIZENS)); return TRUE; }
void CSplitterBar::OnLButtonDown(UINT, CPoint) { SetCapture(); }
void CSplitterBar::OnMouseMove(UINT flags, CPoint point) {
    if (GetCapture() == this && (flags & MK_LBUTTON) && move) { ClientToScreen(&point); GetParent()->ScreenToClient(&point); move(point); }
}
void CSplitterBar::OnLButtonUp(UINT, CPoint) { if (GetCapture() == this) ReleaseCapture(); }
void CSplitterBar::OnCancelMode() { if (GetCapture() == this) ReleaseCapture(); CWnd::OnCancelMode(); }

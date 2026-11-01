#include "workspace_tabs.h"
#include "localization.h"
#include <algorithm>
#include <cstdlib>
int CWorkspaceTabs::Scale(int pixels) const {
    CClientDC dc(const_cast<CWorkspaceTabs*>(this)); return MulDiv(pixels,dc.GetDeviceCaps(LOGPIXELSX),96);
}
CRect CWorkspaceTabs::ActionBounds(int index,Action action) const {
    CRect bounds; if (index < 0 || index >= GetItemCount() || !GetItemRect(index,&bounds)) return CRect();
    if (action == Action::None || (action == Action::Lock && (!lock_state || !lock_state(index).available))) return CRect();
    const auto width = Scale(22); const auto right = bounds.right - Scale(4) - (action == Action::Lock ? width : 0);
    return CRect(right-width,bounds.top+Scale(2),right,bounds.bottom-Scale(2));
}
CWorkspaceTabs::Action CWorkspaceTabs::ActionAt(int index,CPoint point) const {
    if (ActionBounds(index,Action::Close).PtInRect(point)) return Action::Close;
    return ActionBounds(index,Action::Lock).PtInRect(point) ? Action::Lock : Action::None;
}
std::wstring CWorkspaceTabs::ActionText(int index,Action action) const {
    if (action == Action::Close) return simple_editor::ui::Localize(L"Close this tab");
    const auto state = lock_state ? lock_state(index) : LockState{};
    if (action != Action::Lock || !state.available) return L"";
    return simple_editor::ui::Localize(!state.enabled ? L"Document is busy. Wait until the current operation finishes." :
        state.locked ? L"Unlock editing" : L"Lock editing (read-only)");
}
void CWorkspaceTabs::RefreshActions() {
    if (!GetSafeHwnd()) return;
    if (tooltip_.GetSafeHwnd()) tooltip_.Activate(FALSE);
    hovered_ = -1; hover_action_ = Action::None; Invalidate(FALSE);
}
void CWorkspaceTabs::Hover(int index,Action action) {
    const auto text = ActionText(index,action);
    if (index == hovered_ && action == hover_action_ && text == tooltip_text_) return;
    if (tooltip_.GetSafeHwnd()) { tooltip_.Activate(FALSE); tooltip_.DelTool(this,1); }
    hovered_ = index; hover_action_ = action; tooltip_text_ = text; Invalidate(FALSE);
    if (text.empty()) return;
    if (!tooltip_.GetSafeHwnd()) {
        if (!tooltip_.Create(this,TTS_ALWAYSTIP | TTS_NOPREFIX)) return;
        tooltip_.SetMaxTipWidth(Scale(420)); tooltip_.SetFont(GetFont());
    }
    const auto bounds = ActionBounds(index,action);
    tooltip_.AddTool(this,tooltip_text_.c_str(),bounds,1); tooltip_.Activate(TRUE);
}
void CWorkspaceTabs::DrawItem(LPDRAWITEMSTRUCT item) {
    if (item->itemID >= static_cast<UINT>(GetItemCount())) return;
    CDC dc; dc.Attach(item->hDC); const auto saved = dc.SaveDC();
    const int index = static_cast<int>(item->itemID); CRect bounds(item->rcItem); bounds.DeflateRect(1,1);
    const bool selected = index == GetCurSel();
    dc.IntersectClipRect(bounds); dc.FillSolidRect(bounds,GetSysColor(selected ? COLOR_WINDOW : COLOR_BTNFACE));
    dc.SetBkMode(TRANSPARENT); dc.SetTextColor(GetSysColor(COLOR_BTNTEXT)); if (GetFont()) dc.SelectObject(GetFont());
    wchar_t label[2048]{}; TCITEM tab{}; tab.mask = TCIF_TEXT; tab.pszText = label; tab.cchTextMax = _countof(label); GetItem(index,&tab);
    const auto state = lock_state ? lock_state(index) : LockState{};
    CRect text = bounds; text.left += Scale(7); text.right = ActionBounds(index,state.available ? Action::Lock : Action::Close).left - Scale(2);
    dc.DrawText(label,text,DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (selected && GetFocus() == this && !(item->itemState & ODS_NOFOCUSRECT)) dc.DrawFocusRect(text);
    for (const auto action : {Action::Lock,Action::Close}) {
        const auto box = ActionBounds(index,action); if (box.IsRectEmpty()) continue;
        const bool enabled = action == Action::Close || state.enabled;
        const bool hot = index == hovered_ && action == hover_action_;
        if (hot && enabled) dc.FillSolidRect(box,GetSysColor(COLOR_3DLIGHT));
        const auto color = !enabled ? GetSysColor(COLOR_GRAYTEXT) : hot && action == Action::Close ? RGB(180,45,45) : GetSysColor(COLOR_BTNTEXT);
        CPen pen(PS_SOLID,(std::max)(1,Scale(1)),color); const auto old_pen = dc.SelectObject(&pen);
        const auto old_brush = dc.SelectStockObject(NULL_BRUSH); const auto center = box.CenterPoint();
        const auto x = center.x,y = center.y;
        if (action == Action::Close) {
            const auto half = Scale(4); dc.MoveTo(x-half,y-half); dc.LineTo(x+half+1,y+half+1); dc.MoveTo(x+half,y-half); dc.LineTo(x-half-1,y+half+1);
        } else {
            const auto shift = state.locked ? 0 : Scale(3);
            dc.MoveTo(x-Scale(3)+shift,y-Scale(1)); dc.LineTo(x-Scale(3)+shift,y-Scale(5)); dc.LineTo(x-Scale(1)+shift,y-Scale(7));
            dc.LineTo(x+Scale(1)+shift,y-Scale(7)); dc.LineTo(x+Scale(3)+shift,y-Scale(5)); dc.LineTo(x+Scale(3)+shift,y-Scale(1));
            CRect body(x-Scale(5),y-Scale(1),x+Scale(5)+1,y+Scale(6)+1);
            dc.FillSolidRect(body,GetSysColor(selected ? COLOR_WINDOW : COLOR_BTNFACE)); dc.Rectangle(body);
            dc.MoveTo(x,y+Scale(1)); dc.LineTo(x,y+Scale(4));
        }
        dc.SelectObject(old_brush); dc.SelectObject(old_pen);
    }
    dc.RestoreDC(saved); dc.Detach();
}
LRESULT CWorkspaceTabs::WindowProc(UINT message,WPARAM wparam,LPARAM lparam) {
    CPoint point(static_cast<short>(LOWORD(lparam)),static_cast<short>(HIWORD(lparam)));
    if (message == TCM_DELETEITEM || message == TCM_DELETEALLITEMS) {
        pressed_ = -1; pressed_action_ = Action::None; dragging_ = false;
        if (GetCapture() == this) ReleaseCapture(); RefreshActions();
    }
    if (message == WM_SETCURSOR && LOWORD(lparam) == HTCLIENT) {
        CPoint cursor; ::GetCursorPos(&cursor); ScreenToClient(&cursor); const auto index = Hit(cursor); const auto action = ActionAt(index,cursor);
        if (action == Action::Close || (action == Action::Lock && lock_state && lock_state(index).enabled)) { ::SetCursor(::LoadCursor(nullptr,IDC_HAND)); return TRUE; }
    }
    if (message == WM_MOUSEMOVE) {
        const auto index = Hit(point); Hover(index,ActionAt(index,point));
        if (!tracking_) { TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,GetSafeHwnd(),0}; tracking_ = ::TrackMouseEvent(&track) != FALSE; }
    }
    if (tooltip_.GetSafeHwnd() && message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) {
        MSG relay{}; relay.hwnd = GetSafeHwnd(); relay.message = message; relay.wParam = wparam; relay.lParam = lparam; relay.time = GetMessageTime();
        relay.pt = point; ClientToScreen(&relay.pt); tooltip_.RelayEvent(&relay);
    }
    if (message == WM_MOUSELEAVE) { tracking_ = false; Hover(-1,Action::None); }
    if (message == WM_MBUTTONUP) { const auto index = Hit(point); if (index >= 0 && close) close(index); return 0; }
    if (message == WM_CONTEXTMENU) {
        int index; if (point.x == -1 && point.y == -1) { index = GetCurSel(); CRect bounds; if (index >= 0) { GetItemRect(index,bounds); point = bounds.CenterPoint(); ClientToScreen(&point); } }
        else { auto local = point; ScreenToClient(&local); index = Hit(local); }
        if (index >= 0 && context) context(index,point); return 0;
    }
    if (message == WM_LBUTTONDOWN) {
        pressed_ = Hit(point); press_point_ = point; dragging_ = false; pressed_action_ = ActionAt(pressed_,point);
        if (pressed_action_ != Action::None) { SetCapture(); return 0; }
    }
    if (message == WM_MOUSEMOVE && (wparam & MK_LBUTTON) && pressed_ >= 0 && pressed_action_ == Action::None && !dragging_ && (std::abs(point.x - press_point_.x) >= GetSystemMetrics(SM_CXDRAG) || std::abs(point.y - press_point_.y) >= GetSystemMetrics(SM_CYDRAG))) { dragging_ = true; SetCapture(); }
    if (message == WM_LBUTTONUP) {
        const int from = pressed_,to = Hit(point); const bool dragged = dragging_; const auto action = pressed_action_;
        pressed_ = -1; dragging_ = false; pressed_action_ = Action::None;
        if (action != Action::None) {
            if (GetCapture() == this) ReleaseCapture(); RefreshActions();
            if (from == to && ActionAt(to,point) == action) {
                if (action == Action::Close && close) close(to);
                else if (action == Action::Lock && lock_state && lock_state(to).enabled && toggle_lock) toggle_lock(to);
            }
            return 0;
        }
        if (GetCapture() == this) ReleaseCapture(); const auto result = CTabCtrl::WindowProc(message,wparam,lparam); if (dragged && from >= 0 && to >= 0 && from != to && reorder) reorder(from,to); return result;
    }
    if (message == WM_CAPTURECHANGED || message == WM_CANCELMODE) { pressed_ = -1; dragging_ = false; pressed_action_ = Action::None; }
    if (message == WM_DESTROY && tooltip_.GetSafeHwnd()) tooltip_.DestroyWindow();
    const auto result = CTabCtrl::WindowProc(message,wparam,lparam);
    if (message == WM_SETFONT) SetPadding(CSize(Scale(30),Scale(3)));
    if (message == WM_SIZE || message == WM_SETFONT) RefreshActions();
    return result;
}

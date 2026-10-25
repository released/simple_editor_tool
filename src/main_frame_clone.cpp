#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"
void CMainFrame::ToggleTextClone() {
    if (IsBusy() || comparison_session_ || !document_.IsOpen()) return;
    if (!text_clone_.GetSafeHwnd()) {
        if (!text_clone_.Create(WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL,CRect(),this,504)) return;
        text_clone_.Call(SCI_SETDOCPOINTER,0,text_.Call(SCI_GETDOCPOINTER)); text_clone_.SetLimitText(32 * 1024 * 1024);
        text_clone_.SetFont(&text_font_); text_clone_.ConfigureLanguage(document_.Path()); text_clone_.SetTheme(content_theme_); text_clone_.SetWrap(text_.Call(SCI_GETWRAPMODE) != SC_WRAP_NONE); text_clone_.SetWhitespace(text_.Call(SCI_GETVIEWWS) != SCWS_INVISIBLE); text_clone_.SetTabWidth(static_cast<unsigned>(text_.Call(SCI_GETTABWIDTH)),text_.Call(SCI_GETUSETABS) != 0);
        text_clone_.selection_changed = [this] { if (::GetFocus() == text_clone_.GetSafeHwnd()) { clone_active_ = true; if (workspace_) workspace_->TrackNavigation(this); if (!IsBusy()) status_.SetPaneText(0,text_clone_.Status().c_str()); } }; text_clone_.wheel_zoom = [this](short delta) { ZoomWheel(1,delta); };
    }
    dual_view_ = !dual_view_; if (!dual_view_) { clone_active_ = false; text_.SetFocus(); } ShowPage(1); Layout();
    status_.SetPaneText(0,dual_view_ ? L"Two views share one Text draft and Undo history. Carets and scrolling remain independent." : L"Single Text view.");
}

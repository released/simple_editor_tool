#include "main_frame.h"
#include "ui/text_pager.h"
void CMainFrame::OnTextPager() {
    if (IsBusy() || !document_.IsOpen() || comparison_session_ || !ApplyTextEdits()) return; std::wstring error; auto snapshot = document_.Snapshot(&error); if (!snapshot) { status_.SetPaneText(0,error.c_str()); return; }
    CTextPager dialog(this,std::move(snapshot),requested_encoding_); path_dialog_ = true; dialog.DoModal(); path_dialog_ = false;
}

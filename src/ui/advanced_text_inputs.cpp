#include "advanced_compare.h"
#include <algorithm>

bool CAdvancedCompareView::SetPastedInputs(bool enabled) {
    if (enabled && !input_left_.GetSafeHwnd()) {
        const DWORD style = WS_CHILD | WS_BORDER | WS_HSCROLL | WS_VSCROLL |
            WS_TABSTOP | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_WANTRETURN;
        if (!input_left_.Create(style,CRect(),this,582) || !input_right_.Create(style,CRect(),this,583) ||
            !input_left_label_.Create(L"Left text - paste A here (Ctrl+V)",WS_CHILD,CRect(),this,584) ||
            !input_right_label_.Create(L"Right text - paste B here (Ctrl+V)",WS_CHILD,CRect(),this,585)) {
            for (CWnd* control : {static_cast<CWnd*>(&input_left_),static_cast<CWnd*>(&input_right_),static_cast<CWnd*>(&input_left_label_),static_cast<CWnd*>(&input_right_label_)})
                if (control->GetSafeHwnd()) control->DestroyWindow();
            Status(L"Could not create the pasted-text inputs."); return false;
        }
        for (auto* input : {&input_left_,&input_right_}) {
            input->SetLimitText(16 * 1024 * 1024);
            input->SetFont(&content_font_); input->SetTheme(theme_);
            input->Call(SCI_SETPASTECONVERTENDINGS,0);
            input->wheel_zoom = [this](short delta) { if (wheel_zoom) wheel_zoom(delta); };
        }
        input_left_label_.SetFont(&ui_font_); input_right_label_.SetFont(&ui_font_);
        SetTextViewPreferences(wrap_text_,whitespace_,tab_width_,use_tabs_);
    }
    const bool changed_mode = pasted_inputs_ != enabled;
    if (changed_mode) { read_only_snapshots_ = false; snapshot_protection_.clear(); snapshot_codec_ = {}; }
    pasted_inputs_ = enabled;
    if (changed_mode) InvalidateInputs();
    Layout();
    if (enabled) Status(L"Paste text A on the left and text B on the right, then press Compare. No input files are needed.");
    return true;
}

void CAdvancedCompareView::LayoutPastedInputs(int width,int height,int* top) {
    if (!input_left_.GetSafeHwnd()) return;
    const bool visible = pasted_inputs_ && mode_ == 2;
    for (CWnd* control : {static_cast<CWnd*>(&input_left_),static_cast<CWnd*>(&input_right_),static_cast<CWnd*>(&input_left_label_),static_cast<CWnd*>(&input_right_label_)})
        control->ShowWindow(visible ? SW_SHOW : SW_HIDE);
    if (!visible) return;
    const int half = width / 2, input_height = (std::clamp)(height / 3,60,240);
    input_left_label_.MoveWindow(0,0,(std::max)(0,half - 3),20);
    input_right_label_.MoveWindow(half + 3,0,(std::max)(0,width - half - 3),20);
    input_left_.MoveWindow(0,21,(std::max)(0,half - 3),input_height);
    input_right_.MoveWindow(half + 3,21,(std::max)(0,width - half - 3),input_height);
    *top = input_height + 26;
}

bool CAdvancedCompareView::HasPastedInputs() const {
    return input_left_.GetSafeHwnd() &&
        (input_left_.Call(SCI_GETLENGTH) || input_right_.Call(SCI_GETLENGTH));
}

CCodeEditor* CAdvancedCompareView::FocusedInput() {
    if (!pasted_inputs_ || mode_ != 2) return nullptr;
    for (auto* input : {&input_left_,&input_right_})
        if (input->GetSafeHwnd() && ::GetFocus() == input->GetSafeHwnd()) return input;
    return nullptr;
}

void CAdvancedCompareView::SetSnapshotInputs(const std::wstring& left,const std::wstring& right,const std::wstring& path,const simple_editor::core::TextCodec& codec,const simple_editor::core::FileStamp& stamp) {
    if (!SetPastedInputs(true)) return;
    read_only_snapshots_ = true; snapshot_codec_ = codec; snapshot_protection_ = {{path,{stamp.volume,stamp.identity}}};
    input_left_.SetReadOnly(FALSE); input_right_.SetReadOnly(FALSE);
    input_left_.SetWindowTextW(left.c_str()); input_right_.SetWindowTextW(right.c_str()); input_left_.ConfigureLanguage(path); input_right_.ConfigureLanguage(path);
    input_left_.SetReadOnly(TRUE); input_right_.SetReadOnly(TRUE);
    input_left_label_.SetWindowTextW(L"Current Text draft (read-only snapshot)"); input_right_label_.SetWindowTextW(L"Fresh disk version (read-only snapshot)");
    InvalidateInputs(); StartPastedText();
}

void CAdvancedCompareView::OnPastedInputChanged() {
    if (job_.valid() && job_kind_ == JobKind::Text) cancelled_ = true;
    InvalidateInputs();
    if (changed) changed();
}

void CAdvancedCompareView::StartPastedText(bool notify_equal) {
    if (Busy() || !pasted_inputs_ || mode_ != 2) return;
    constexpr LRESULT limit = 16 * 1024 * 1024;
    if (input_left_.Call(SCI_GETLENGTH) > limit || input_right_.Call(SCI_GETLENGTH) > limit) {
        Status(L"Pasted-text comparison supports up to 16 MiB of UTF-8 text per side. Inputs are retained; reduce the text before comparing."); return;
    }
    const auto a = input_left_.Utf8(), b = input_right_.Utf8();
    auto left = CCodeEditor::FromUtf8(a), right = CCodeEditor::FromUtf8(b); std::wstring error;
    if ((!a.empty() && left.empty()) || (!b.empty() && right.empty()) ||
        !simple_editor::core::ValidateText(left,&error) || !simple_editor::core::ValidateText(right,&error)) {
        Status(L"Pasted inputs must be valid Unicode text without embedded NUL. Inputs are retained."); return;
    }
    StartTextJob(nullptr,nullptr,nullptr,std::move(left),std::move(right),true,notify_equal);
}
bool CAdvancedCompareView::LoadPastedPair(const std::wstring& left,const std::wstring& right) {
    if (Busy() || !pasted_inputs_ || read_only_snapshots_ || HasPastedInputs()) return false;
    input_left_.SetWindowTextW(left.c_str()); input_right_.SetWindowTextW(right.c_str());
    InvalidateInputs(); StartPastedText(); return job_.valid();
}

void CAdvancedCompareView::SwapPastedInputs() {
    if (Busy() || !pasted_inputs_) return;
    const auto left = input_left_.Call(SCI_GETDOCPOINTER), right = input_right_.Call(SCI_GETDOCPOINTER);
    input_left_.Call(SCI_ADDREFDOCUMENT,0,left); input_right_.Call(SCI_ADDREFDOCUMENT,0,right);
    input_left_.Call(SCI_SETDOCPOINTER,0,right); input_right_.Call(SCI_SETDOCPOINTER,0,left);
    input_left_.Call(SCI_RELEASEDOCUMENT,0,left); input_right_.Call(SCI_RELEASEDOCUMENT,0,right);
    OnPastedInputChanged();
}

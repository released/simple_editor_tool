#include "conversion_dialog.h"
#include "../resource.h"
#include "../core/line_merge.h"
#include <algorithm>
using namespace simple_editor::core;
BEGIN_MESSAGE_MAP(CConversionDialog,CDialog)
    ON_WM_TIMER()
    ON_WM_SIZE()
END_MESSAGE_MAP()
CConversionDialog::CConversionDialog(CWnd* owner,bool may_apply) : CDialog(IDD_DATA_CONVERSION,owner),may_apply_(may_apply) {}
BOOL CConversionDialog::OnInitDialog() {
    CDialog::OnInitDialog(); input_format_.SubclassDlgItem(3100,this); output_format_.SubclassDlgItem(3101,this);
    for (auto* combo : {&input_format_,&output_format_}) for (const auto label : {L"Hex byte pairs",L"Base64 (standard)",L"UTF-8 text"}) combo->AddString(label);
    input_format_.SetCurSel(0); output_format_.SetCurSel(1); font_.CreatePointFont(100,L"Consolas");
    for (const auto item : {std::make_pair(&input_,3103),std::make_pair(&output_,3104)}) { CRect bounds; GetDlgItem(item.second)->GetWindowRect(bounds); ScreenToClient(bounds); GetDlgItem(item.second)->ShowWindow(SW_HIDE); item.first->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL | WS_HSCROLL,bounds,this,item.second + 10); item.first->SetFont(&font_); item.first->SetTheme(1); item.first->SetLimitText(item.second == 3103 ? 32 * 1024 * 1024 : 128 * 1024); }
    output_.SetReadOnly(TRUE); SetTimer(1,50,nullptr); UpdateActions(); SetDlgItemTextW(3105,may_apply_ ? L"Paste input and choose Preview. Decoded bytes: at most 16 MiB." : L"Copy-only: applying requires an editable file Hex selection of at most 16 MiB."); layout_.Initialize(*this,3113,3114); layout_.Fit(*this); return TRUE;
}
bool CConversionDialog::Current() const { return valid_ && generation_ == input_.Generation() && input_kind_ == input_format_.GetCurSel() && output_kind_ == output_format_.GetCurSel(); }
void CConversionDialog::UpdateActions() { GetDlgItem(3102)->EnableWindow(!job_.valid()); GetDlgItem(3106)->EnableWindow(Current() && !job_.valid()); GetDlgItem(IDOK)->EnableWindow(may_apply_ && Current() && !job_.valid()); }
void CConversionDialog::Preview() {
    if (job_.valid()) return; const auto text = input_.Utf8(); generation_ = input_.Generation(); input_kind_ = input_format_.GetCurSel(); output_kind_ = output_format_.GetCurSel(); cancelled_ = false; valid_ = false;
    const auto input = static_cast<DataFormat>(input_kind_),output = static_cast<DataFormat>(output_kind_);
    job_ = std::async(std::launch::async,[this,text,input,output]() { Converted value; SaveControl control; control.cancelled = &cancelled_; ConvertData(text,input,output,control,&value.value,&value.error); return value; }); UpdateActions(); SetDlgItemTextW(3105,L"Converting in background...");
}
void CConversionDialog::OnTimer(UINT_PTR id) {
    if (id != 1) { CDialog::OnTimer(id); return; }
    if (!job_.valid() || job_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) return;
    Converted value; try { value = job_.get(); } catch (...) { value.error = L"Conversion exceeded available memory; no bytes applied."; }
    const bool same = generation_ == input_.Generation() && input_kind_ == input_format_.GetCurSel() && output_kind_ == output_format_.GetCurSel();
    if (!same || cancelled_) { SetDlgItemTextW(3105,L"Input/options changed; preview discarded. Run Preview again."); valid_ = false; }
    else if (!value.error.empty()) { SetDlgItemTextW(3105,value.error.c_str()); valid_ = false; }
    else { result = std::move(value.value); valid_ = true; auto size = (std::min<std::size_t>)(65536,result.output.size()); while (size < result.output.size() && size && (static_cast<unsigned char>(result.output[size]) & 0xC0) == 0x80) --size;
        output_.SetWindowTextW(TextForEditControl(CCodeEditor::FromUtf8(result.output.substr(0,size))).c_str()); const auto status = std::to_wstring(result.bytes.size()) + L" decoded bytes; " + std::to_wstring(result.output.size()) + L" formatted UTF-8 bytes." + (size < result.output.size() ? L" Preview truncated; Copy includes full output." : L" Complete preview."); SetDlgItemTextW(3105,status.c_str()); }
    UpdateActions();
}
void CConversionDialog::Copy() {
    if (!Current() || job_.valid()) return; const auto text = CCodeEditor::FromUtf8(result.output); const auto bytes = (text.size() + 1) * sizeof(wchar_t); HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE,bytes); if (!memory) return; auto* data = GlobalLock(memory); if (!data) { GlobalFree(memory); return; } memcpy(data,text.c_str(),bytes); GlobalUnlock(memory);
    if (!::OpenClipboard(GetSafeHwnd())) { GlobalFree(memory); return; } if (!::EmptyClipboard() || !::SetClipboardData(CF_UNICODETEXT,memory)) GlobalFree(memory); ::CloseClipboard();
}
BOOL CConversionDialog::OnCommand(WPARAM wparam,LPARAM lparam) {
    const auto id = LOWORD(wparam),code = HIWORD(wparam);
    if (id == 3102) { Preview(); return TRUE; } if (id == 3106) { Copy(); return TRUE; }
    if ((id == 3113 && code == EN_CHANGE) || ((id == 3100 || id == 3101) && code == CBN_SELCHANGE)) { cancelled_ = true; valid_ = false; if (GetDlgItem(3106)) UpdateActions(); }
    return CDialog::OnCommand(wparam,lparam);
}
void CConversionDialog::OnOK() { if (may_apply_ && Current() && !job_.valid()) { KillTimer(1); CDialog::OnOK(); } }
void CConversionDialog::OnCancel() { cancelled_ = true; if (job_.valid()) job_.wait(); KillTimer(1); CDialog::OnCancel(); }
void CConversionDialog::OnSize(UINT type,int width,int height) { CDialog::OnSize(type,width,height); layout_.Apply(*this); }

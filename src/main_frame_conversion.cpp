#include "main_frame.h"
#include "ui/conversion_dialog.h"
using namespace simple_editor::core;
void CMainFrame::ConversionDialog() {
    if (IsBusy()) return;
    const bool may_apply = active_tab_ == 0 && !comparison_session_ && document_.IsOpen() && !MutationBlocked() && hex_.SelectionLength() && hex_.SelectionLength() <= 16 * 1024 * 1024;
    const auto revision = revision_,offset = hex_.SelectionStart(),length = hex_.SelectionLength(); CConversionDialog dialog(this,may_apply); path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false;
    if (answer == IDOK && may_apply) ApplyConvertedBytes(dialog.result.bytes,revision,offset,length);
}
bool CMainFrame::ApplyConvertedBytes(const std::vector<std::uint8_t>& bytes,std::uint64_t revision,std::uint64_t offset,std::uint64_t length) {
    std::wstring error;
    if (IsBusy() || MutationBlocked() || comparison_session_ || revision != revision_ || active_tab_ != 0 || !length || length > 16 * 1024 * 1024 || bytes.size() > 16 * 1024 * 1024 || offset > document_.Size() || length > document_.Size() - offset || !document_.ValidateSource(true,&error)) { status_.SetPaneText(0,L"Captured editable Hex range is no longer valid; no bytes applied."); return false; }
    auto snapshot = document_.Snapshot(&error,true); if (!snapshot) { status_.SetPaneText(0,error.c_str()); return false; }
    job_revision_ = revision_; job_kind_ = EditorJobKind::Transform; cancelled_ = false;
    job_ = std::async(std::launch::async,[this,bytes,offset,length,snapshot = std::move(snapshot)]() mutable { EditorJobResult result; result.kind = EditorJobKind::Transform; if (!cancelled_ && snapshot->Replace(offset,length,bytes) && snapshot->ValidateSource(true,&result.error)) { result.document = std::move(snapshot); result.count = 1; result.message = L"Decoded bytes applied to the captured Hex selection. One Undo restores it; file is not saved."; } else if (result.error.empty()) result.error = L"Conversion application cancelled or failed; original document retained."; return result; }); return true;
}

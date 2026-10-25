#include "main_frame.h"
#include "resource.h"
#include "ui/template_dialog.h"
using namespace simple_editor::core;
bool CMainFrame::StartTemplate(const std::wstring& path,std::uint64_t base) {
    auto* view = ActiveHex(); if (IsBusy() || !view || (view == &hex_ && !ApplyTextEdits())) return false;
    const auto* source = view == &hex_ ? &document_ : view == &compare_left_ ? &compare_left_document_ : &compare_right_document_; std::wstring error; auto snapshot = source->Snapshot(&error); if (!snapshot) { status_.SetPaneText(0,error.c_str()); return false; }
    const auto target = view == &hex_ ? 0 : view == &compare_left_ ? 1 : 2; cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_revision_ = revision_; job_kind_ = EditorJobKind::Template;
    try { job_ = std::async(std::launch::async,[this,path,base,target,snapshot = std::move(snapshot)]() mutable {
        EditorJobResult result; result.kind = EditorJobKind::Template; result.found = target; BinaryDocument schema; std::vector<std::uint8_t> bytes; std::wstring definition; TextCodec codec;
        if (!schema.Open(path,&result.error)) return result;
        if (schema.Size() > 1024 * 1024) { result.error = L"Template definitions are limited to 1 MiB."; return result; }
        if (!schema.Read(0,static_cast<std::size_t>(schema.Size()),&bytes) || !schema.ValidateSource(true,&result.error) || !DecodeText(bytes,TextEncoding::Utf8,&definition,&codec,&result.error)) { if (result.error.empty()) result.error = L"Could not read the UTF-8 template."; return result; }
        result.fields = std::make_shared<BinaryTemplateResult>(); SaveControl control; control.cancelled = &cancelled_; if (!InspectBinaryTemplate(*snapshot,definition,base,control,result.fields.get(),&result.error)) result.fields.reset(); result.document = std::move(snapshot); return result;
    }); } catch (...) { status_.SetPaneText(0,L"Could not start template inspection."); return false; } status_.SetPaneText(0,L"Reading binary field template. Esc cancels."); return true;
}
void CMainFrame::OnTemplate() {
    if (IsBusy() || !ActiveHex()) return; CFileDialog file(TRUE,L"settpl",nullptr,OFN_FILEMUSTEXIST,L"Binary field template (*.settpl)|*.settpl|All files (*.*)|*.*||",this); path_dialog_ = true; const auto answer = file.DoModal(); path_dialog_ = false; if (answer != IDOK) return;
    auto* view = ActiveHex(); if (!view) return; CAddressDialog address(UINT64_MAX,this); path_dialog_ = true; const auto accepted = address.DoModal(); path_dialog_ = false; if (accepted == IDOK) StartTemplate(file.GetPathName().GetString(),address.address);
}
void CMainFrame::FinishTemplate(EditorJobResult result) {
    if (cancelled_ || job_revision_ != revision_) { status_.SetPaneText(0,L"Template results cancelled or obsolete."); return; }
    if (!result.error.empty() || !result.fields || !result.document || !result.document->ValidateSource(true,&result.error)) { status_.SetPaneText(0,result.error.c_str()); return; }
    CTemplateDialog dialog(this,*result.fields); path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false;
    if (answer == IDOK && job_revision_ == revision_ && result.document->ValidateSource(true,&result.error)) { const auto& field = result.fields->fields[dialog.selected]; ShowPage(result.found == 0 ? 0 : 2); auto* view = result.found == 0 ? &hex_ : result.found == 1 ? &compare_left_ : &compare_right_; view->SelectRange(field.offset,field.length); view->SetFocus(); } status_.SetPaneText(0,L"Binary field template inspected; source bytes retained.");
}

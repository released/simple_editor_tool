#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"
#include "core/binary_merge.h"
#include <sstream>
using namespace simple_editor::core;
bool CMainFrame::StartBinaryMerge(const std::wstring& path,bool right_into_left,const std::set<std::size_t>& selected) {
    if (IsBusy() || AdvancedActive() || selected.empty()) return false; std::wstring error; std::vector<BinaryPatch> patches;
    if (!BuildBinaryPatches(pending_differences_,selected,right_into_left,compare_left_document_.Size(),compare_right_document_.Size(),&patches,&error)) { status_.SetPaneText(0,error.c_str()); return false; }
    auto baseline = (right_into_left ? compare_left_document_ : compare_right_document_).Snapshot(&error), donor = (right_into_left ? compare_right_document_ : compare_left_document_).Snapshot(&error);
    if (!baseline || !donor) { status_.SetPaneText(0,error.c_str()); return false; }
    job_locked_paths_ = {path}; if (workspace_ && !workspace_->LockPaths(job_locked_paths_,true,&job_)) { job_locked_paths_.clear(); status_.SetPaneText(0,L"Output has unsaved changes or is busy."); return false; } job_locks_held_ = workspace_ != nullptr;
    cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_kind_ = EditorJobKind::Export;
    try { job_ = std::async(std::launch::async,[this,path,patches,baseline = std::move(baseline),donor = std::move(donor)]() { EditorJobResult result; result.kind = EditorJobKind::Export; SaveControl control; control.cancelled = &cancelled_; control.progress = [this](unsigned p) { progress_ = p; }; if (MergeBinaryRanges(*baseline,*donor,patches,path,control,&result.error)) { result.message = L"Selected binary ranges merged to: " + path; result.open_path = path; } return result; }); }
    catch (...) { ReleaseJobLocks(); status_.SetPaneText(0,L"Could not start binary merging."); return false; } return true;
}
void CMainFrame::OnBinaryMerge(UINT command) {
    if (IsBusy() || AdvancedActive() || pending_differences_.empty()) { status_.SetPaneText(0,L"Run a binary comparison and select differences in its list first."); return; }
    std::set<std::size_t> selected; for (int row = differences_.GetNextItem(-1,LVNI_SELECTED); row >= 0; row = differences_.GetNextItem(row,LVNI_SELECTED)) selected.insert(static_cast<std::size_t>(row));
    if (selected.empty()) { status_.SetPaneText(0,L"Select difference rows first. Ctrl+A selects the displayed differences."); return; }
    const bool direction = command == ID_BINARY_TAKE_RIGHT; std::vector<BinaryPatch> patches; std::wstring error;
    if (!BuildBinaryPatches(pending_differences_,selected,direction,compare_left_document_.Size(),compare_right_document_.Size(),&patches,&error)) { status_.SetPaneText(0,error.c_str()); return; }
    std::wostringstream preview; preview << (direction ? L"Baseline: left. Selected data comes from right.\n" : L"Baseline: right. Selected data comes from left.\n") << selected.size() << L" selected byte differences in " << patches.size() << L" range(s). Only selected displayed differences are included.\n\nBaseline offset: removed bytes -> donor offset: copied bytes\n";
    for (std::size_t i = 0; i < (std::min<std::size_t>)(20,patches.size()); ++i) { const auto& patch = patches[i]; preview << L"0x" << std::hex << patch.offset << L": " << std::dec << patch.remove << L" -> 0x" << std::hex << patch.source << L": " << std::dec << patch.length << L"\n"; }
    preview << L"\nContinue to choose a separate output file?"; path_dialog_ = true; const auto answer = AfxMessageBox(preview.str().c_str(),MB_OKCANCEL | MB_ICONQUESTION); path_dialog_ = false; if (answer != IDOK) return;
    CFileDialog output(FALSE,L"bin",L"merged.bin",OFN_OVERWRITEPROMPT,L"All files (*.*)|*.*||",this); path_dialog_ = true; const auto accepted = output.DoModal(); path_dialog_ = false; if (accepted == IDOK) StartBinaryMerge(output.GetPathName().GetString(),direction,selected);
}

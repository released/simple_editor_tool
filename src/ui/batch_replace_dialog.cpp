#include "batch_replace_dialog.h"
#include "feature_dialogs.h"
#include "editor_dialogs.h"
#include "../resource.h"
#include <ShlObj.h>
using namespace simple_editor::core;
using namespace simple_editor::services;
BEGIN_MESSAGE_MAP(CBatchReplaceDialog,CDialog)
    ON_WM_TIMER()
    ON_WM_DESTROY()
END_MESSAGE_MAP()
CBatchReplaceDialog::CBatchReplaceDialog(CWnd* owner) : CDialog(IDD_BATCH_REPLACE,owner) {}
CBatchReplaceDialog::~CBatchReplaceDialog() { cancel_ = true; if (planning_.valid()) planning_.wait(); if (applying_.valid()) applying_.wait(); ReleaseLocks(); }
BOOL CBatchReplaceDialog::OnInitDialog() { CDialog::OnInitDialog(); syntax_.SubclassDlgItem(2803,this); encoding_.SubclassDlgItem(2804,this); for (auto text : {L"Literal text",L"Escaped text",L"Scintilla POSIX regex (single line)"}) syntax_.AddString(text); syntax_.SetCurSel(0); for (auto text : {L"Auto",L"UTF-8",L"UTF-16 LE",L"UTF-16 BE",L"Big5",L"System"}) encoding_.AddString(text); encoding_.SetCurSel(0); SetDlgItemTextW(2807,L"*.txt;*.cpp;*.h;*.ini;*.json;*.csv;*.log"); CheckDlgButton(2805,BST_CHECKED); CheckDlgButton(2809,BST_CHECKED); SetTimer(1,50,nullptr); return TRUE; }
BOOL CBatchReplaceDialog::OnCommand(WPARAM wp,LPARAM lp) { if (LOWORD(wp) == 2810 && !planning_.valid() && !applying_.valid()) { BROWSEINFOW browse{}; browse.hwndOwner = GetSafeHwnd(); browse.lpszTitle = L"Choose the saved-text folder to preview"; browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE; const auto folder = SHBrowseForFolderW(&browse); wchar_t path[MAX_PATH]{}; if (folder && SHGetPathFromIDListW(folder,path)) SetDlgItemTextW(2800,path); if (folder) CoTaskMemFree(folder); return TRUE; } return CDialog::OnCommand(wp,lp); }
void CBatchReplaceDialog::OnOK() {
    if (planning_.valid() || applying_.valid()) return; CString root,find,replacement,include,exclude; GetDlgItemTextW(2800,root); GetDlgItemTextW(2801,find); GetDlgItemTextW(2802,replacement); GetDlgItemTextW(2807,include); GetDlgItemTextW(2808,exclude); if (root.IsEmpty() || find.IsEmpty()) { SetDlgItemTextW(2811,L"Choose an existing folder and enter a nonempty search pattern."); return; }
    MultiSearchRequest request; request.text.find = find.GetString(); request.text.replacement = replacement.GetString(); request.text.syntax = syntax_.GetCurSel(); request.text.match_case = IsDlgButtonChecked(2805) != 0; request.text.whole_word = IsDlgButtonChecked(2806) != 0; request.encoding = static_cast<TextEncoding>(encoding_.GetCurSel()); request.recursive = IsDlgButtonChecked(2809) != 0; request.include = include.GetString(); request.exclude = exclude.GetString(); StartPlan(root.GetString(),request);
}
bool CBatchReplaceDialog::StartPlan(const std::filesystem::path& root,const MultiSearchRequest& request) {
    if (planning_.valid() || applying_.valid()) return false; cancel_ = false; progress_ = 0; closing_ = false; GetDlgItem(IDOK)->EnableWindow(FALSE); SetDlgItemTextW(2811,L"Planning replacements from saved disk text. No files are being changed.");
    try { planning_ = std::async(std::launch::async,[this,root,request] { Planning result; SaveControl control; control.cancelled = &cancel_; control.progress = [this](unsigned p) { progress_ = p; }; PlanFolderReplace(root,request,control,&result.plan,&result.error); return result; }); } catch (...) { GetDlgItem(IDOK)->EnableWindow(TRUE); return false; } return true;
}
void CBatchReplaceDialog::ReleaseLocks() { if (!locked_.empty() && lock_paths) lock_paths(locked_,false,{}); locked_.clear(); }
bool CBatchReplaceDialog::StartApply(const std::vector<std::size_t>& selected) {
    if (planning_.valid() || applying_.valid() || selected.empty()) return false; std::vector<FileStamp> expected; for (auto index : selected) { if (index >= plan_.files.size()) { locked_.clear(); return false; } locked_.push_back((plan_.root / plan_.files[index].relative).wstring()); expected.push_back(plan_.files[index].stamp); }
    if (lock_paths && !lock_paths(locked_,true,expected)) { locked_.clear(); SetDlgItemTextW(2811,L"A selected file is dirty or busy in another editor. Save/close it, then preview again."); return false; }
    cancel_ = false; progress_ = 0; GetDlgItem(IDOK)->EnableWindow(FALSE); SetDlgItemTextW(2811,L"Applying selected files. Esc/Close cancels before the next file commits; committed files retain backups/journal."); std::shared_ptr<BatchReplacePlan> plan;
    try { plan = std::make_shared<BatchReplacePlan>(std::move(plan_)); applying_ = std::async(std::launch::async,[this,plan,selected] { SaveControl control; control.cancelled = &cancel_; control.progress = [this](unsigned p) { progress_ = p; }; return ApplyFolderReplace(*plan,selected,control); }); } catch (...) { if (plan) plan_ = std::move(*plan); ReleaseLocks(); GetDlgItem(IDOK)->EnableWindow(TRUE); return false; } return true;
}
void CBatchReplaceDialog::Review() {
    if (plan_.failed) { std::wstring text = std::to_wstring(plan_.failed) + L" input(s) were skipped; these files will not be replaced. First 100 errors:\r\n"; for (const auto& error : plan_.errors) text += error + L"\r\n"; CTextResultDialog errors(this,L"Replacement planning errors",text); errors.DoModal(); }
    if (plan_.files.empty()) { SetDlgItemTextW(2811,L"No eligible changes. Existing files were retained."); return; }
    std::vector<std::vector<std::wstring>> rows; for (const auto& file : plan_.files) rows.push_back({file.relative.wstring(),std::to_wstring(file.count),std::to_wstring(file.stamp.size),std::to_wstring(file.output.size())}); CReviewList review(this,L"Review replacements - select files to apply",{L"Relative path",L"Replacements",L"Old bytes",L"New bytes"},std::move(rows));
    review.details = [&](std::size_t index) { const auto& file = plan_.files[index]; CTextResultDialog text(this,L"Before / after replacement preview",L"Before (first 65,536 characters):\r\n" + file.before + L"\r\n\r\nAfter (first 65,536 characters):\r\n" + file.after + L"\r\n\r\nFull validated output is retained for application; preview text may be shortened."); text.DoModal(); };
    if (review.DoModal() == IDOK) StartApply(review.selected);
}
void CBatchReplaceDialog::Tick() {
    if (planning_.valid() && planning_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) { Planning result; try { result = planning_.get(); } catch (...) { result.error = L"Planning failed: memory/path/I/O error."; } GetDlgItem(IDOK)->EnableWindow(TRUE); if (closing_) { CDialog::OnCancel(); return; } if (cancel_ || !result.error.empty()) { SetDlgItemTextW(2811,result.error.empty() ? L"Planning cancelled; no originals changed." : result.error.c_str()); return; } plan_ = std::move(result.plan); Review(); }
    if (applying_.valid() && applying_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) { BatchReplaceResult result; try { result = applying_.get(); } catch (...) { result.error = L"Application failed; inspect recovery directories before retrying."; } ReleaseLocks(); const auto summary = std::to_wstring(result.sync.completed) + L" file(s) committed.\r\nBackup: " + result.sync.backup.wstring() + L"\r\nJournal: " + result.sync.journal.wstring() + L"\r\nRetained staging: " + result.staging.wstring() + L"\r\n" + result.error + (result.sync.cancelled || cancel_ ? L"\r\nCancelled. Already committed files remain committed." : L""); SetDlgItemTextW(2811,summary.c_str()); CTextResultDialog report(this,L"Replacement application result",summary); report.DoModal(); if (closing_) CDialog::OnCancel(); else EndDialog(IDOK); }
}
void CBatchReplaceDialog::OnTimer(UINT_PTR id) { if (id == 1) Tick(); else CDialog::OnTimer(id); }
void CBatchReplaceDialog::OnCancel() { if (planning_.valid() || applying_.valid()) { closing_ = true; cancel_ = true; SetDlgItemTextW(2811,L"Cancelling at the next safe file boundary..."); return; } CDialog::OnCancel(); }
void CBatchReplaceDialog::OnDestroy() { KillTimer(1); cancel_ = true; if (planning_.valid()) planning_.wait(); if (applying_.valid()) applying_.wait(); ReleaseLocks(); CDialog::OnDestroy(); }

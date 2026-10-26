#include "advanced_compare.h"
#include <algorithm>
using namespace simple_editor::core;
FolderOptions CAdvancedCompareView::CurrentFolderOptions() const {
    CString include,exclude; include_.GetWindowTextW(include); exclude_.GetWindowTextW(exclude);
    FolderOptions options{recursive_.GetCheck() == BST_CHECKED,include.IsEmpty() ? L"*" : include.GetString(),exclude.GetString()};
    options.criterion = static_cast<FolderCriterion>((std::max)(0,folder_criterion_.GetCurSel())); options.time_tolerance_seconds = static_cast<unsigned>((std::max)(0,folder_tolerance_.GetCurSel())); options.text_rules = folder_text_rules_; return options;
}
bool CAdvancedCompareView::RefreshSelectedFolders() {
    if (Busy() || !scan_ || stale_ || mode_ != 4) { Status(L"Run a folder comparison before refreshing selected rows."); return false; }
    const auto options = CurrentFolderOptions(); const auto& old = scan_options_;
    if (options.recursive != old.recursive || options.include != old.include || options.exclude != old.exclude || options.criterion != old.criterion || options.time_tolerance_seconds != old.time_tolerance_seconds || options.text_rules != old.text_rules) { Status(L"Folder options changed. Run full Compare before refreshing selected rows."); return false; }
    std::vector<std::filesystem::path> scopes; refresh_selected_.clear(); refresh_expanded_.clear(); refresh_top_.clear();
    for (int row = rows_.GetNextItem(-1,LVNI_SELECTED); row >= 0; row = rows_.GetNextItem(row,LVNI_SELECTED)) if (static_cast<std::size_t>(row) < visible_.size()) { const auto& path = scan_->rows[visible_[row]].relative; scopes.push_back(path); refresh_selected_.insert(path.wstring()); }
    if (scopes.empty() || scopes.size() > 128) { Status(L"Select 1-128 file/folder rows, or use full Compare."); return false; }
    for (auto row : tree_expanded_) if (row < scan_->rows.size()) refresh_expanded_.insert(scan_->rows[row].relative.wstring());
    const auto top = rows_.GetTopIndex(); if (top >= 0 && static_cast<std::size_t>(top) < visible_.size()) refresh_top_ = scan_->rows[visible_[top]].relative.wstring();
    const auto scan = scan_; cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_kind_ = JobKind::Refresh; SetBusy(true); Status(L"Refreshing selected paths; other results retain their previous scan state.");
    try { job_ = std::async(std::launch::async,[this,scan,options,scopes] { Result result; result.kind = JobKind::Refresh; result.scan = std::make_shared<FolderScan>(); if (!RefreshFolderSelection(*scan,options,scopes,cancelled_,[this](unsigned p) { progress_ = p; },result.scan.get(),&result.error) && result.error.empty()) result.error = L"Selected refresh failed or was cancelled; old results retained."; return result; }); }
    catch (...) { SetBusy(false); Status(L"Could not start selected refresh; old results retained."); return false; } return true;
}

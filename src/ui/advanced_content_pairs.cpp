#include "advanced_compare.h"
#include "feature_dialogs.h"
using namespace simple_editor::core;
void CAdvancedCompareView::StartContentPairing() {
    if (Busy() || mode_ != 4 || !scan_ || stale_) { Status(L"Run a folder comparison first. Content pairing examines left-only/right-only files without changing synchronization plans."); return; } const auto scan = scan_; cancelled_ = false; job_kind_ = JobKind::Pairing; progress_ = 0; SetBusy(true);
    try { job_ = std::async(std::launch::async,[this,scan] { Result result; result.kind = JobKind::Pairing; result.pairs = std::make_shared<std::vector<ContentPair>>(); SaveControl control; control.cancelled = &cancelled_; control.progress = [this](unsigned p) { progress_ = p; }; FindContentPairs(*scan,control,result.pairs.get(),&result.error); return result; }); } catch (...) { SetBusy(false); Status(L"Could not start content pairing."); }
}
void CAdvancedCompareView::FinishContentPairing(const std::vector<ContentPair>& pairs) {
    std::vector<std::vector<std::wstring>> rows; for (const auto& pair : pairs) rows.push_back({pair.left.wstring(),pair.right.wstring(),pair.state,std::to_wstring(pair.left_count),std::to_wstring(pair.right_count)}); if (rows.empty()) { Status(L"No equal-content rename/move candidates were found among unmatched files."); return; }
    CReviewList dialog(this,L"Read-only content pairing",{L"Left path",L"Right path",L"State",L"Left candidates",L"Right candidates"},std::move(rows)); modal_ = true; const auto answer = dialog.DoModal(); modal_ = false;
    if (answer == IDOK && !dialog.selected.empty()) { const auto& pair = pairs[dialog.selected[0]]; FileStamp left,right; std::wstring error; if (pair.left_count != 1 || pair.right_count != 1) { Status(L"Duplicate content has multiple candidates; no pair was chosen."); return; } if (!InspectFile(pair.left,&left,&error) || !InspectFile(pair.right,&right,&error) || !SameFileState(left,pair.left_stamp) || !SameFileState(right,pair.right_stamp)) { Status(L"A candidate changed; run content pairing again."); return; } if (open_pair) open_pair(pair.left.wstring(),pair.right.wstring(),0); }
    Status(L"Content pairing is read-only; filenames and sync plans were retained.");
}

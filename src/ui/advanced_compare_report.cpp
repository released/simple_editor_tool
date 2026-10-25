#include "advanced_compare.h"
using namespace simple_editor::core;
std::wstring CAdvancedCompareView::RawOutputText() const { CString text; output_.GetWindowTextW(text); return TextFromEditControl(generated_,std::wstring(text.GetString(),text.GetLength())); }
bool CAdvancedCompareView::StartTextReport(const std::wstring& path,TextReportFormat format) {
    if (Busy() || !model_ || mode_ == 4 || stale_) return false;
    if (manual_view_) { Status(L"Clear manual anchors and compare again before exporting a text report."); return false; }
    locked_paths_ = {path}; locked_stamps_.clear(); if (lock_paths && !lock_paths(locked_paths_,true,locked_stamps_)) { locked_paths_.clear(); Status(L"The report output is busy or has unsaved edits."); return false; } locks_held_ = static_cast<bool>(lock_paths);
    const auto model = model_; const auto inputs = report_inputs_; const auto options = line_options_; const auto note = std::wstring(L"ignore-space=") + (options.ignore_space ? L"yes" : L"no") + L"\r\nignore-case=" + (options.ignore_case ? L"yes" : L"no") + L"\r\nignore-eol=" + (options.ignore_eol ? L"yes" : L"no") + L"\r\n" + text_rules_; cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_kind_ = JobKind::Report; SetBusy(true);
    try { job_ = std::async(std::launch::async,[this,path,format,model,inputs,options,note]() { Result result; result.kind = JobKind::Report; std::wstring left,right; for (const auto& line : model->left) { left += line.text; left += line.ending; } for (const auto& line : model->right) { right += line.text; right += line.ending; }
        SaveControl control; control.cancelled = &cancelled_; control.progress = [this](unsigned p) { progress_ = p; }; TextReportStats stats; result.saved = ExportTextReport(left,right,inputs,path,format,control,&stats,&result.error,false,&options,note); if (result.saved) result.text = L"Text report saved: " + path + L". Blocks: " + std::to_wstring(stats.blocks) + L", right additions/replacements: " + std::to_wstring(stats.added) + L", left removals/replacements: " + std::to_wstring(stats.removed); return result;
    }); } catch (...) { SetBusy(false); ReleaseLocks(); Status(L"Could not start text report generation."); return false; } return true;
}
void CAdvancedCompareView::ExportReportDialog() {
    if (Busy() || !model_ || mode_ == 4 || stale_) { Status(L"Run a text comparison first."); return; }
    CFileDialog dialog(FALSE,L"html",L"text-comparison.html",OFN_OVERWRITEPROMPT,L"HTML comparison (*.html)|*.html|Text summary (*.txt)|*.txt|Unified patch (*.patch)|*.patch||",this); modal_ = true; const auto selected = dialog.DoModal(); modal_ = false; if (selected == IDOK) StartTextReport(dialog.GetPathName().GetString(),static_cast<TextReportFormat>(dialog.m_ofn.nFilterIndex - 1));
}

#include "main_frame.h"
#include "ui/statistics_dialog.h"
using namespace simple_editor::core;
bool CMainFrame::StartStatistics() {
    auto* view = ActiveHex(); if (IsBusy() || !view || (view == &hex_ && !ApplyTextEdits())) { status_.SetPaneText(0,L"Choose a file or comparison Hex view first."); return false; }
    const auto* source = view == &hex_ ? &document_ : view == &compare_left_ ? &compare_left_document_ : &compare_right_document_; std::wstring error; auto snapshot = source->Snapshot(&error); if (!snapshot) { status_.SetPaneText(0,error.c_str()); return false; }
    const auto offset = view->SelectionLength() ? view->SelectionStart() : 0,length = view->SelectionLength() ? view->SelectionLength() : snapshot->Size(); statistics_view_ = view; job_revision_ = revision_; job_kind_ = EditorJobKind::Statistics; cancelled_ = false; progress_ = 0;
    job_ = std::async(std::launch::async,[this,offset,length,snapshot = std::move(snapshot)]() { EditorJobResult result; result.kind = EditorJobKind::Statistics; result.found = offset; result.analysis = std::make_shared<BinaryAnalysis>(); SaveControl control; control.cancelled = &cancelled_; control.progress = [this](unsigned value) { progress_ = value; }; if (AnalyzeBinary(*snapshot,offset,length,control,result.analysis.get(),&result.error)) snapshot->ValidateSource(true,&result.error); return result; }); return true;
}
void CMainFrame::FinishStatistics(EditorJobResult result) {
    auto* view = statistics_view_; const auto* source = view == &hex_ ? &document_ : view == &compare_left_ ? &compare_left_document_ : &compare_right_document_;
    std::wstring error; if (cancelled_ || revision_ != job_revision_ || !view || !result.error.empty() || !source->ValidateSource(true,&error)) { status_.SetPaneText(0,!result.error.empty() ? result.error.c_str() : error.empty() ? L"Statistics cancelled or source changed; results discarded." : error.c_str()); return; }
    CStatisticsDialog dialog(this,*result.analysis,result.found); path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false;
    if (answer == IDOK) { if (revision_ == job_revision_ && source->ValidateSource(true,&error)) { view->SelectRange(dialog.selected_offset,dialog.selected_length); view->SetFocus(); } else status_.SetPaneText(0,L"Statistics source changed; block navigation discarded."); }
}

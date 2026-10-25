#include "workspace_frame.h"
#include "main_frame.h"
#include "resource.h"
#include "ui/command_palette.h"
void CWorkspaceFrame::TrackNavigation(CMainFrame* frame) {
    if (navigating_ || destroying_ || active_session_ < 0 || active_session_ >= static_cast<int>(sessions_.size()) || sessions_[active_session_] != frame || frame->comparison_session_ || !frame->document_.IsOpen() || (frame->active_tab_ != 0 && frame->active_tab_ != 1)) return;
    NavigationPoint point; point.session = frame->workspace_session_id_; point.page = frame->active_tab_; point.clone = point.page == 1 && frame->dual_view_ && frame->clone_active_; auto* editor = point.clone ? static_cast<CCodeEditor*>(&frame->text_clone_) : static_cast<CCodeEditor*>(&frame->text_); point.generation = point.page ? editor->Generation() : frame->revision_; point.offset = point.page ? static_cast<std::uint64_t>(editor->Call(SCI_GETCURRENTPOS)) : frame->hex_.CursorOffset();
    if (!navigation_.empty()) { const auto& current = navigation_[navigation_index_]; if (current.session == point.session && current.page == point.page && current.clone == point.clone && current.generation == point.generation && current.offset == point.offset) return; navigation_.resize(navigation_index_ + 1); }
    navigation_.push_back(point); if (navigation_.size() > 256) navigation_.erase(navigation_.begin()); navigation_index_ = navigation_.size() - 1;
}
void CWorkspaceFrame::NavigateHistory(bool previous) {
    if (navigation_.empty() || destroying_ || workspace_dialog_) return;
    auto index = navigation_index_; for (std::size_t tries = 0; tries < navigation_.size(); ++tries) { if (previous) { if (!index) return; --index; } else { if (index + 1 >= navigation_.size()) return; ++index; } const auto point = navigation_[index]; const auto found = std::find_if(sessions_.begin(),sessions_.end(),[&](auto* frame) { return frame->workspace_session_id_ == point.session; }); if (found == sessions_.end()) continue; auto* frame = *found; auto* editor = point.clone ? static_cast<CCodeEditor*>(&frame->text_clone_) : static_cast<CCodeEditor*>(&frame->text_); if (frame->comparison_session_ || frame->IsBusy() || (point.page && (!editor->GetSafeHwnd() || (point.clone && !frame->dual_view_))) || (point.page ? editor->Generation() : frame->revision_) != point.generation || !frame->document_.ValidateSource(true) || point.offset > (point.page ? static_cast<std::uint64_t>(editor->Call(SCI_GETLENGTH)) : frame->document_.Size())) continue;
        navigating_ = true; ActivateSession(static_cast<int>(found - sessions_.begin())); frame->ShowPage(point.page); if (point.page) { frame->clone_active_ = point.clone; editor->Call(SCI_SETEMPTYSELECTION,point.offset); editor->Call(SCI_SCROLLCARET); editor->SetFocus(); } else { frame->hex_.GoTo(point.offset); frame->hex_.SetFocus(); } navigation_index_ = index; navigating_ = false; return;
    }
}
void CWorkspaceFrame::OnWorkflowRoot(UINT command) {
    if (command == ID_WORKFLOW_BACK || command == ID_WORKFLOW_FORWARD) { NavigateHistory(command == ID_WORKFLOW_BACK); return; }
    if (active_session_ >= 0 && active_session_ < static_cast<int>(sessions_.size())) { sessions_[active_session_]->OnWorkflow(command); return; }
    if (command == ID_WORKFLOW_PALETTE && !workspace_dialog_) { auto commands = CollectPaletteCommands(GetMenu()); for (auto& item : commands) if (item.id == ID_WORKFLOW_PALETTE) item.enabled = false; CCommandPalette dialog(this,std::move(commands)); workspace_dialog_ = true; const auto answer = dialog.DoModal(); workspace_dialog_ = false; if (answer == IDOK) SendMessage(WM_COMMAND,dialog.selected); }
}

#include "workspace_frame.h"
#include "main_frame.h"
#include <algorithm>
void CWorkspaceFrame::OpenFolderPair(CMainFrame* parent,const std::wstring& left,const std::wstring& right,int mode) {
    if (!parent || std::find(sessions_.begin(),sessions_.end(),parent) == sessions_.end()) return;
    const auto key = parent->advanced_.FolderPairKey(left,right); const auto id = parent->workspace_session_id_,generation = parent->advanced_.FolderGeneration();
    for (std::size_t i = 0; i < sessions_.size(); ++i) { auto* frame = sessions_[i]; if (frame->folder_parent_id_ != id || frame->folder_relative_ != key || key.empty()) continue; CString a,b; frame->compare_left_path_.GetWindowTextW(a); frame->compare_right_path_.GetWindowTextW(b); if (frame->comparison_session_ && frame->displayed_mode_ == mode && a == left.c_str() && b == right.c_str()) { frame->folder_parent_generation_ = generation; ActivateSession(static_cast<int>(i)); return; } }
    if (left.empty() || right.empty()) { if (!OpenPath(left.empty() ? right : left)) return; } else NewComparisonInputs(left,right,L"",mode,true);
    if (active_session_ < 0 || key.empty()) return; auto* child = sessions_[active_session_]; if (child == parent) return; child->folder_parent_id_ = id; child->folder_parent_generation_ = generation; child->folder_relative_ = key;
}
bool CWorkspaceFrame::NavigateFolderPair(CMainFrame* child,int direction) {
    if (!child || std::find(sessions_.begin(),sessions_.end(),child) == sessions_.end()) return false;
    CMainFrame* parent = nullptr; int index = -1; for (std::size_t i = 0; i < sessions_.size(); ++i) if (sessions_[i]->workspace_session_id_ == child->folder_parent_id_) { parent = sessions_[i]; index = static_cast<int>(i); break; }
    if (!parent) { child->status_.SetPaneText(0,L"The parent folder comparison is no longer open."); return false; }
    if (!direction) { ActivateSession(index); return true; }
    if (parent->advanced_.Busy() || parent->advanced_.FolderGeneration() != child->folder_parent_generation_) { child->status_.SetPaneText(0,L"Parent results changed or are busy. Return to the folder comparison before choosing another file."); return false; }
    CAdvancedCompareView::FolderFile file; if (!parent->advanced_.AdjacentFolderFile(child->folder_relative_,direction,&file)) { child->status_.SetPaneText(0,L"No adjacent differing file in the parent's current visible order."); return false; }
    OpenFolderPair(parent,file.left,file.right,file.mode); return true;
}

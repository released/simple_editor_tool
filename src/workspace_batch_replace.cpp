#include "workspace_frame.h"
#include "ui/batch_replace_dialog.h"
void CWorkspaceFrame::OnBatchReplace() {
    if (destroying_ || workspace_dialog_) return; CBatchReplaceDialog dialog(this); dialog.lock_paths = [this,&dialog](const std::vector<std::wstring>& paths,bool acquire,const std::vector<simple_editor::core::FileStamp>& expected) { return LockPaths(paths,acquire,&dialog,expected); }; workspace_dialog_ = true; dialog.DoModal(); workspace_dialog_ = false;
}

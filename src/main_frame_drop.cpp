#include "main_frame.h"
#include "workspace_frame.h"
#include <Shellapi.h>

bool CMainFrame::AcceptComparisonDrop(const std::vector<std::wstring>& paths, CPoint screen) {
    if (active_tab_ != 2 || displayed_mode_ == 4 || displayed_mode_ == 8 || paths.size() != 1) return false;
    CRect page = page_rect_; ClientToScreen(page); if (!page.PtInRect(screen)) return false;
    if (IsBusy()) { status_.SetPaneText(0,L"An operation is running. Cancel it or wait for completion."); return true; }
    simple_editor::core::BinaryDocument opened; std::wstring error;
    if (!opened.Open(paths.front(),&error)) { status_.SetPaneText(0,error.c_str()); return true; }
    CRect base; compare_base_path_.GetWindowRect(base);
    auto* target = displayed_mode_ == 3 && base.PtInRect(screen) ? &compare_base_path_ : screen.x < page.CenterPoint().x ? &compare_left_path_ : &compare_right_path_;
    const bool previous = suppress_compare_path_change_; suppress_compare_path_change_ = true;
    SetPathComboText(*target,opened.Path()); suppress_compare_path_change_ = previous;
    if (workspace_) workspace_->RememberRecent(opened.Path());
    OnComparePathChanged(); RefreshComparisonPaths(); return true;
}
void CMainFrame::OnDropFiles(HDROP drop) {
    CPoint point; if (DragQueryPoint(drop,&point)) ClientToScreen(&point);
    std::vector<std::wstring> paths; const auto count = DragQueryFileW(drop,0xFFFFFFFF,nullptr,0);
    for (UINT i = 0; i < count; ++i) {
        const auto length = DragQueryFileW(drop,i,nullptr,0); std::vector<wchar_t> path(length + 1);
        if (DragQueryFileW(drop,i,path.data(),length + 1)) paths.emplace_back(path.data());
    }
    DragFinish(drop);
    if (AcceptComparisonDrop(paths,point)) return;
    if (workspace_) { for (const auto& path : paths) workspace_->OpenPath(path); }
    else if (!paths.empty()) OpenPath(paths.front());
}

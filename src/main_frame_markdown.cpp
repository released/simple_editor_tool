#include "main_frame.h"
#include "resource.h"
#include "ui/localization.h"
#include <algorithm>
#include <cwctype>
#include <filesystem>
bool CMainFrame::MarkdownPreviewAvailable() const {
    if (comparison_session_ || !document_.IsOpen()) return false;
    auto extension = std::filesystem::path(document_.Path()).extension().wstring();
    std::transform(extension.begin(),extension.end(),extension.begin(),[](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return extension == L".md" || extension == L".markdown" || extension == L".mdown";
}
bool CMainFrame::MarkdownPreviewActive() const { return active_tab_ == 1 && markdown_enabled_ && MarkdownPreviewAvailable(); }
void CMainFrame::LayoutTextPage(CRect page) {
    const bool show = MarkdownPreviewActive();
    if (show && !markdown_preview_.GetSafeHwnd()) {
        if (!markdown_preview_.Create(this)) { markdown_enabled_ = false; text_.MoveWindow(page); return; }
        const auto cls = AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_3DSHADOW + 1),nullptr);
        markdown_splitter_.Create(cls,nullptr,WS_CHILD,CRect(),this,6213);
        markdown_splitter_.move = [this](CPoint point) { markdown_ratio_ = (std::clamp)((point.x - page_rect_.left) / static_cast<double>((std::max)(1,page_rect_.Width())),0.25,0.75); Layout(); };
        markdown_source_caption_.Create(L"Markdown source",WS_CHILD,CRect(),this,6214); markdown_source_caption_.SetFont(&ui_font_);
    }
    if (markdown_preview_.GetSafeHwnd()) markdown_preview_.ShowWindow(show ? SW_SHOW : SW_HIDE);
    if (markdown_splitter_.GetSafeHwnd()) markdown_splitter_.ShowWindow(show ? SW_SHOW : SW_HIDE);
    if (markdown_source_caption_.GetSafeHwnd()) markdown_source_caption_.ShowWindow(show ? SW_SHOW : SW_HIDE);
    if (show) {
        const int middle = page.left + static_cast<int>(page.Width() * markdown_ratio_),top = (std::min)(page.bottom,page.top + 26);
        markdown_source_caption_.SetWindowTextW(simple_editor::ui::Localize(L"Markdown source").c_str());
        markdown_source_caption_.MoveWindow(page.left + 6,page.top + 2,(std::max<LONG>)(0,middle - page.left - 12),(std::max<LONG>)(0,top - page.top - 2));
        text_.MoveWindow(page.left,top,(std::max<LONG>)(0,middle - page.left - 3),(std::max<LONG>)(0,page.bottom - top));
        markdown_splitter_.MoveWindow(middle - 2,page.top,5,page.Height());
        markdown_preview_.MoveWindow(middle + 3,page.top,(std::max<LONG>)(0,page.right - middle - 3),page.Height());
        if (text_clone_.GetSafeHwnd()) text_clone_.ShowWindow(SW_HIDE);
    } else if (dual_view_ && text_clone_.GetSafeHwnd()) {
        CRect left = page,right = page; left.right = page.left + page.Width() / 2 - 2; right.left = left.right + 4; text_.MoveWindow(left); text_clone_.MoveWindow(right);
    } else text_.MoveWindow(page);
}
void CMainFrame::TickMarkdownPreview() {
    if (!markdown_preview_.GetSafeHwnd()) return;
    const bool visible = MarkdownPreviewActive() && IsWindowVisible() && !::IsIconic(::GetAncestor(GetSafeHwnd(),GA_ROOT));
    if (visible && markdown_source_caption_.GetSafeHwnd()) {
        const auto label = simple_editor::ui::Localize(L"Markdown source"); CString current; markdown_source_caption_.GetWindowTextW(current);
        if (current != label.c_str()) markdown_source_caption_.SetWindowTextW(label.c_str());
    }
    markdown_preview_.Tick(text_,revision_,document_.Path(),visible && !IsBusy(),content_theme_,content_font_tenths_,zoom_percent_[1]);
    if (visible) markdown_preview_.SynchronizeScroll(text_,revision_);
}
void CMainFrame::OnMarkdownPreview() {
    if (IsBusy() || !MarkdownPreviewAvailable()) return;
    markdown_enabled_ = !markdown_enabled_; ShowPage(1); Layout();
}

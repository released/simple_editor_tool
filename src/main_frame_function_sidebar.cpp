#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"
#include <algorithm>
simple_editor::core::OutlineLanguage CMainFrame::FunctionSidebarLanguage() const {
    if (comparison_session_ || !text_editable_ || !document_.IsOpen()) return simple_editor::core::OutlineLanguage::None;
    const auto lexer = text_.SyntaxName(); const auto language = simple_editor::core::OutlineLanguageFor(document_.Path(),lexer,syntax_mode_ == L"cpp");
    return language == simple_editor::core::OutlineLanguage::None && lexer == "cpp" ? simple_editor::core::OutlineLanguage::Cpp : language;
}
void CMainFrame::LayoutFunctionSidebar(CRect& page) {
    const bool show = active_tab_ == 1 && FunctionSidebarLanguage() != simple_editor::core::OutlineLanguage::None;
    if (!show) { if (function_sidebar_.GetSafeHwnd()) function_sidebar_.ShowWindow(SW_HIDE); if (function_splitter_.GetSafeHwnd()) function_splitter_.ShowWindow(SW_HIDE); return; }
    if (!function_sidebar_.GetSafeHwnd()) {
        if (!function_sidebar_.Create(this)) return;
        const auto cls = AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_3DSHADOW + 1),nullptr);
        function_splitter_.Create(cls,nullptr,WS_CHILD,CRect(),this,IDC_FUNCTION_SPLITTER);
        function_splitter_.move = [this](CPoint point) { function_sidebar_width_ = (std::max)(120,static_cast<int>(point.x - page_rect_.left)); Layout(); };
        function_sidebar_.navigate = [this](std::size_t line) { const auto language = FunctionSidebarLanguage(); if (IsBusy() || language == simple_editor::core::OutlineLanguage::None || active_tab_ != 1 || !function_sidebar_.Current(text_,revision_,document_.Path(),language)) return; auto* editor = ActiveEditor(); if (editor) { editor->GoToLine(line); editor->SetFocus(); } };
    }
    const auto width = (std::clamp)(function_sidebar_width_,120,(std::max)(120,page.Width() - (dual_view_ ? 246 : 166)));
    function_sidebar_.MoveWindow(page.left,page.top,width,page.Height()); function_sidebar_.ShowWindow(SW_SHOW);
    function_splitter_.MoveWindow(page.left + width,page.top,5,page.Height()); function_splitter_.ShowWindow(SW_SHOW); page.left = (std::min)(page.right,page.left + width + 5);
}
void CMainFrame::TickFunctionSidebar() {
    if (!function_sidebar_.GetSafeHwnd()) return; const auto language = FunctionSidebarLanguage();
    function_sidebar_.Tick(text_,revision_,document_.Path(),language,language != simple_editor::core::OutlineLanguage::None && active_tab_ == 1 && (GetStyle() & WS_VISIBLE) && !IsBusy(),content_theme_);
}

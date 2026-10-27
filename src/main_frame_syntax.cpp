#include "main_frame.h"
#include "workspace_frame.h"
#include "ui/feature_dialogs.h"
#include "core/syntax_profiles.h"
#include "ui/localization.h"
using namespace simple_editor::core;
void CMainFrame::ConfigureFileSyntax() {
    const auto path = document_.Path(); const auto mode = syntax_mode_.empty() && workspace_ ? workspace_->SyntaxForPath(path) : syntax_mode_;
    for (auto* editor : {&text_,&text_clone_}) if (editor->GetSafeHwnd()) editor->ConfigureLanguage(path,mode);
    if (!function_sidebar_.Current(text_,revision_,path,FunctionSidebarLanguage())) function_sidebar_.InvalidateSource(); Layout();
}
void CMainFrame::ChooseSyntax() {
    if (IsBusy() || comparison_session_ || !document_.IsOpen()) { status_.SetPaneText(0,L"Choose a normal Text document before changing its syntax mode."); return; }
    std::vector<std::vector<std::wstring>> rows; for (const auto& mode : SyntaxModes()) rows.push_back({simple_editor::ui::Localize(mode.label)});
    CReviewList dialog(this,L"Syntax mode",{L"Language"},rows); dialog.single_select = true; dialog.accept_label = L"Use language"; path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false;
    if (answer == IDOK && !dialog.selected.empty() && dialog.selected[0] < SyntaxModes().size()) { syntax_mode_ = SyntaxModes()[dialog.selected[0]].id; ConfigureFileSyntax(); status_.SetPaneText(0,L"Syntax changed; document text, encoding and Undo are retained."); }
}
void CMainFrame::ChooseSyntaxAssociations() {
    if (IsBusy() || !workspace_) return;
    CFeatureInput dialog(this,L"Syntax extension associations",L"One extension=language per line, for example .conf=JSON or .inc=C/C++. Supported languages: C/C++, Python, JSON, HTML/XML, PowerShell, Markdown, SQL, YAML, Shell, Batch, Diff/Patch, CMake, Plain text, Automatic. Existing per-document overrides take priority.",workspace_->SyntaxAssociations());
    dialog.validate = [](const auto& text,std::wstring* error) { std::map<std::wstring,std::wstring> mappings; return ParseSyntaxAssociations(text,&mappings,error); };
    path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; std::wstring error;
    if (answer == IDOK && !workspace_->SetSyntaxAssociations(dialog.value,&error)) UiMessageBox(error.c_str(),MB_ICONWARNING);
}

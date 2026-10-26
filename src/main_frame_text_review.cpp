#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"
#include "ui/review_dialogs.h"
#include "core/text_review.h"
#include "core/function_outline.h"
#include <chrono>
using namespace simple_editor::core;

bool CMainFrame::StartOutline() {
    auto* editor = ActiveEditor(); if (IsBusy() || !editor) { status_.SetPaneText(0,L"Choose a Text editor before opening its Markdown outline."); return false; }
    const auto input = editor->Utf8(); if (input.size() > 32 * 1024 * 1024) { status_.SetPaneText(0,L"Markdown outline supports up to 32 MiB of text."); return false; }
    function_outline_ = false; job_kind_ = EditorJobKind::Outline; job_editor_ = editor; job_editor_generation_ = editor->Generation(); cancelled_ = false;
    try { job_ = std::async(std::launch::async,[this,text = CCodeEditor::FromUtf8(input)]() { EditorJobResult result; result.kind = EditorJobKind::Outline; result.headings = std::make_shared<std::vector<MarkdownHeading>>(); if (!MarkdownOutline(text,cancelled_,result.headings.get(),&result.truncated)) result.error = L"Outline cancelled or input exceeds its bound."; return result; }); }
    catch (...) { job_editor_ = nullptr; status_.SetPaneText(0,L"Could not start outline analysis."); return false; }
    status_.SetPaneText(0,L"Reading Markdown headings; document edits are retained."); return true;
}
void CMainFrame::FinishOutline(EditorJobResult result) {
    auto* editor = job_editor_; job_editor_ = nullptr;
    if (cancelled_ || !editor || !editor->GetSafeHwnd() || editor->Generation() != job_editor_generation_) { status_.SetPaneText(0,L"Outline is stale or cancelled; repeat the command."); return; }
    if (!result.error.empty() || !result.headings) { status_.SetPaneText(0,result.error.c_str()); return; }
    if (result.headings->empty()) { status_.SetPaneText(0,function_outline_ ? L"No supported C/C++ definitions found. Macros, lambdas and complex declarations may be omitted." : L"No Markdown headings found outside code blocks."); return; }
    CMarkdownOutlineDialog dialog(this,*result.headings); if (function_outline_) dialog.function_list = true; path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false;
    if (answer == IDOK && editor->GetSafeHwnd() && editor->Generation() == job_editor_generation_) editor->GoToLine(dialog.line);
    status_.SetPaneText(0,result.truncated ? L"Outline reached its bounded result or parsing limit." : function_outline_ ? L"C/C++ outline is heuristic. Source text and Undo are retained." : L"Markdown outline preserves text, encoding and Undo.");
}
bool CMainFrame::StartFunctionOutline() {
    auto* editor = ActiveEditor(); if (IsBusy() || !editor || editor->SyntaxName() != "cpp") { status_.SetPaneText(0,L"Choose the C/C++ syntax mode before opening the function list."); return false; }
    const auto input = editor->Utf8(); if (input.size() > 32 * 1024 * 1024) return false;
    function_outline_ = true; job_kind_ = EditorJobKind::Outline; job_editor_ = editor; job_editor_generation_ = editor->Generation(); cancelled_ = false;
    try { job_ = std::async(std::launch::async,[this,text = CCodeEditor::FromUtf8(input)] { EditorJobResult result; result.kind = EditorJobKind::Outline; result.headings = std::make_shared<std::vector<MarkdownHeading>>(); if (!CppFunctionOutline(text,cancelled_,result.headings.get(),&result.truncated)) result.error = L"Function outline cancelled or input exceeds its bounds."; return result; }); }
    catch (...) { job_editor_ = nullptr; return false; } status_.SetPaneText(0,L"Reading C/C++ definitions; document edits are retained."); return true;
}
void CMainFrame::RequestIndentation(bool reset) {
    if (reset) indentation_manual_ = false;
    if (!document_.IsOpen() || comparison_session_ || untitled_) return;
    indentation_requested_ = document_.Path(); if (indentation_io_.valid()) return;
    const auto defaults = workspace_ ? workspace_->IndentationDefaults() : IndentationSettings{}; indentation_running_ = indentation_requested_;
    try { indentation_io_ = std::async(std::launch::async,[path = indentation_running_,defaults]() { return ResolveEditorConfig(path,defaults); }); }
    catch (...) { if (!IsBusy()) status_.SetPaneText(0,L"Could not read EditorConfig; current indentation retained."); }
}
void CMainFrame::TickIndentation() {
    if (!indentation_io_.valid() && document_.IsOpen() && !untitled_ && !comparison_session_ && indentation_requested_ != document_.Path()) RequestIndentation(false);
    if (!indentation_io_.valid() || IsBusy() || indentation_io_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) return;
    EditorConfigResult result; try { result = indentation_io_.get(); } catch (...) { status_.SetPaneText(0,L"EditorConfig could not be read; current indentation retained."); return; }
    if (indentation_running_ != indentation_requested_) { RequestIndentation(false); return; }
    if (indentation_running_ != document_.Path() || !document_.IsOpen() || untitled_ || comparison_session_) return;
    indentation_result_ = std::move(result);
    if (!indentation_manual_) for (auto* editor : {&text_,&text_clone_}) if (editor->GetSafeHwnd()) { const auto& settings = indentation_result_.indentation; editor->SetTabWidth(settings.tab_width,settings.use_tabs); editor->Call(SCI_SETINDENT,settings.indent_width); }
    if (!indentation_result_.warnings.empty()) status_.SetPaneText(0,(L"EditorConfig: " + indentation_result_.warnings.front()).c_str());
    else if (indentation_result_.applied) status_.SetPaneText(0,L"EditorConfig indentation applied. Original encoding/EOL and native Undo retained.");
}

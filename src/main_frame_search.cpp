#include "main_frame.h"
#include "resource.h"
#include "ui/native_search.h"
#include "core/pattern_search.h"
#include "core/search.h"
#include <algorithm>

namespace {
std::uint64_t EncodedCaretOffset(const std::wstring& original,CCodeEditor& editor,std::uint64_t utf8_position,const simple_editor::core::TextCodec& codec) {
    using namespace simple_editor::core;
    const auto displayed = CCodeEditor::FromUtf8(editor.Utf8()), raw = TextFromEditControl(original,displayed); auto remaining = static_cast<std::size_t>(editor.Call(SCI_COUNTCODEUNITS,0,utf8_position)); std::wstring prefix;
    for (const auto& line : SplitLines(raw)) {
        if (remaining <= line.text.size()) { prefix += line.text.substr(0,remaining); break; }
        prefix += line.text; remaining -= line.text.size();
        if (!line.ending.empty()) { if (remaining >= 2) { prefix += line.ending; remaining -= 2; } else { prefix += line.ending.substr(0,1); remaining = 0; } }
        if (!remaining) break;
    }
    std::vector<std::uint8_t> bytes; std::wstring error; return EncodeText(prefix,codec,true,&bytes,&error) ? bytes.size() : UINT64_MAX;
}
}

void CMainFrame::UnlockEditorSearch() { if (!job_editor_) return; if (!IsFileEditor(job_editor_)) advanced_.SearchBusy(false); else SetSaveReadOnly(false); job_editor_ = nullptr; }
void CMainFrame::ExecuteEditorSearch(const SearchRequest& request,CCodeEditor* editor) {
    if (IsBusy() || !editor || request.find.IsEmpty()) return;
    const bool mutation = request.action == SearchAction::Replace || request.action == SearchAction::ReplaceAll;
    if (mutation && editor->Call(SCI_GETREADONLY)) { status_.SetPaneText(0,L"The active text buffer is read-only."); return; }
    NativeSearchOptions options; options.find = request.find.GetString(); options.replacement = request.replacement.GetString(); options.action = static_cast<int>(request.action); options.syntax = request.mode == 3 ? 2 : request.mode == 2 ? 1 : 0; options.match_case = request.match_case; options.whole_word = request.whole_word; options.previous = request.previous; options.wrap = request.wrap;
    const auto input = editor->Utf8(); const auto selected_start = static_cast<std::uint64_t>(editor->ContentPosition(editor->Call(SCI_GETSELECTIONSTART))), selected_end = static_cast<std::uint64_t>(editor->ContentPosition(editor->Call(SCI_GETSELECTIONEND))); auto begin = 0ULL, end = static_cast<unsigned long long>(input.size());
    if (request.selection_only) { if (scope_editor_ != editor || scope_pattern_ != options.find || scope_generation_ != editor->Generation() || selected_start != scope_last_begin_ || selected_end != scope_last_end_) { scope_begin_ = selected_start; scope_end_ = selected_end; } begin = scope_begin_; end = scope_end_; if (begin == end) { status_.SetPaneText(0,L"Select a non-empty search range first."); return; } }
    else { scope_begin_ = 0; scope_end_ = input.size(); }
    scope_editor_ = editor; scope_pattern_ = options.find; scope_generation_ = editor->Generation(); scope_last_begin_ = selected_start; scope_last_end_ = selected_end;
    last_search_ = request; job_editor_ = editor; job_editor_generation_ = editor->Generation(); search_input_size_ = input.size(); job_kind_ = EditorJobKind::TextSearch; cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX;
    if (!IsFileEditor(editor)) advanced_.SearchBusy(true); else editor->SetReadOnly(TRUE);
    try { job_ = std::async(std::launch::async,[this,input,options,begin,end,selected_start,selected_end]() { EditorJobResult result; result.kind = EditorJobKind::TextSearch; result.search = std::make_shared<NativeSearchPlan>(); PlanEditorSearch(input,options,begin,end,selected_start,selected_end,cancelled_,[this](unsigned p) { progress_ = p; },result.search.get(),&result.error); return result; }); }
    catch (...) { UnlockEditorSearch(); status_.SetPaneText(0,L"Could not start the native search worker."); }
}
void CMainFrame::FinishEditorSearch(EditorJobResult result) {
    auto* editor = job_editor_; const bool valid = editor && editor->GetSafeHwnd() && editor->Generation() == job_editor_generation_; UnlockEditorSearch();
    if (!valid || cancelled_) { status_.SetPaneText(0,L"Search cancelled or buffer changed; old results discarded."); return; }
    if (!result.error.empty() || !result.search) { status_.SetPaneText(0,result.error.empty() ? L"Search failed." : result.error.c_str()); return; }
    auto& plan = *result.search;
    if (last_search_.action == SearchAction::FindAll && last_search_.mark_colour >= 0 && last_search_.mark_colour < 3) {
        const auto indicator = static_cast<unsigned>(8 + last_search_.mark_colour); editor->Call(SCI_SETINDICATORCURRENT,indicator); editor->Call(SCI_INDICATORCLEARRANGE,0,editor->Call(SCI_GETLENGTH));
        for (const auto& match : plan.matches) editor->Call(SCI_INDICATORFILLRANGE,editor->NativePosition(match.offset),match.length);
        status_.SetPaneText(0,(std::to_wstring(plan.count) + L" occurrence(s); editing clears marks." + (plan.truncated ? L" First 100,000 highlighted." : L"")).c_str()); return;
    }
    if (plan.changed) { if (editor->Call(SCI_GETREADONLY)) return; editor->ReplaceGenerated(CCodeEditor::FromUtf8(plan.output),0); editor->GetParent()->SendMessage(WM_COMMAND,MAKEWPARAM(editor->GetDlgCtrlID(),EN_CHANGE),reinterpret_cast<LPARAM>(editor->GetSafeHwnd())); if (last_search_.selection_only) { if (plan.output.size() >= search_input_size_) scope_end_ += plan.output.size() - search_input_size_; else scope_end_ -= search_input_size_ - plan.output.size(); } }
    if (plan.found || (plan.changed && last_search_.action == SearchAction::Replace)) { editor->Call(SCI_SETSEL,editor->NativePosition(plan.cursor),editor->NativePosition(plan.cursor + plan.length)); editor->Call(SCI_SCROLLCARET); scope_last_begin_ = plan.cursor; scope_last_end_ = plan.cursor + plan.length; }
    if (plan.found && IsFileEditor(editor)) { match_offset_ = EncodedCaretOffset(text_original_,*editor,plan.cursor,text_codec_); match_pattern_.clear(); if (match_offset_ != UINT64_MAX) hex_.GoTo(match_offset_); }
    scope_generation_ = editor->Generation();
    const auto message = std::to_wstring(plan.count) + (plan.changed ? L" replacement(s). One Undo restores the operation." : L" match(es).") + (plan.wrapped ? L" Wrapped." : L""); status_.SetPaneText(0,message.c_str());
    if (last_search_.action == SearchAction::FindAll) { const auto generation = editor->Generation(); search_results_.navigate = [this,editor,generation](const NativeSearchMatch& match) { if (!editor->GetSafeHwnd() || editor->Generation() != generation) { status_.SetPaneText(0,L"Search results are stale. Run Find All again."); return; } editor->Call(SCI_SETSEL,editor->NativePosition(match.offset),editor->NativePosition(match.offset + match.length)); editor->Call(SCI_SCROLLCARET); editor->SetFocus(); }; search_results_.Show(this,std::move(plan.matches),message + (plan.truncated ? L" First 100,000 shown." : L" Double-click to navigate.")); }
}
void CMainFrame::ExecuteByteSearch(const SearchRequest& request) {
    using namespace simple_editor::core;
    if (active_tab_ == 1 && !AdvancedActive()) ShowPage(0);
    auto* view = ActiveHex(); if (!view || IsBusy() || (view == &hex_ && !ApplyTextEdits())) return;
    const bool mutation = request.action == SearchAction::Replace || request.action == SearchAction::ReplaceAll;
    if (mutation && (view != &hex_ || MutationBlocked())) return;
    BytePattern pattern; std::vector<std::uint8_t> replacement; std::wstring error;
    if (!ParseBytePattern(request.find.GetString(),request.mode == 5,&pattern,&error) || (mutation && !ParseHexBytes(request.replacement.GetString(),&replacement))) { status_.SetPaneText(0,error.empty() ? L"Replacement must contain complete Hex byte pairs." : error.c_str()); return; }
    const auto* source = view == &hex_ ? &document_ : view == &compare_left_ ? &compare_left_document_ : &compare_right_document_;
    auto snapshot = source->Snapshot(&error,mutation); if (!snapshot) { status_.SetPaneText(0,error.c_str()); return; }
    const auto key = std::to_wstring(request.mode) + L":" + request.find.GetString(); const auto cursor = view->SelectionStart(); std::uint64_t begin = 0, end = snapshot->Size();
    if (request.selection_only) { if (byte_search_view_ != view || byte_search_key_ != key || byte_scope_revision_ != revision_ || cursor != byte_last_match_ || view->SelectionLength() != byte_last_length_) { byte_scope_begin_ = cursor; byte_scope_end_ = cursor + view->SelectionLength(); } begin = byte_scope_begin_; end = byte_scope_end_; if (begin == end) { status_.SetPaneText(0,L"Select a non-empty byte search range first."); return; } }
    if (mutation && end - begin > 64ULL * 1024 * 1024) { status_.SetPaneText(0,L"Pattern replacement supports a 64-MiB selected range. Select a smaller range and enable Selection only."); return; }
    const bool repeated = byte_search_view_ == view && byte_search_key_ == key && byte_last_match_ == cursor && byte_last_length_ == view->SelectionLength();
    byte_search_view_ = view; byte_search_key_ = key; byte_scope_revision_ = revision_; last_search_ = request; job_revision_ = revision_; job_kind_ = EditorJobKind::ByteSearch; cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX;
    try { job_ = std::async(std::launch::async,[this,request,pattern,replacement,begin,end,cursor,repeated,snapshot = std::move(snapshot)]() mutable {
        EditorJobResult result; result.kind = EditorJobKind::ByteSearch; result.search = std::make_shared<NativeSearchPlan>(); std::vector<std::uint64_t> offsets; std::uint64_t next = begin;
        const auto collect = [&](std::uint64_t at) { if (at < next) return true; next = at + pattern.bytes.size(); ++result.count;
            if (request.action == SearchAction::ReplaceAll) { if (offsets.size() >= 1000000) { result.error = L"Pattern replacement exceeds 1,000,000 matches; select a smaller range."; return false; } offsets.push_back(at); }
            if (request.action == SearchAction::FindAll && result.search->matches.size() < 100000) { NativeSearchMatch match; match.offset = at; match.length = pattern.bytes.size(); std::vector<std::uint8_t> data; if (snapshot->Read(at,16,&data)) for (auto value : data) { char text[4]{}; sprintf_s(text,"%02X ",value); match.preview += text; } result.search->matches.push_back(std::move(match)); } return !cancelled_;
        };
        const auto progress = [this](unsigned p) { progress_ = p; };
        if (request.action == SearchAction::Replace) { std::vector<std::uint8_t> current; if (cursor >= begin && cursor <= end && pattern.bytes.size() <= end - cursor && snapshot->Read(cursor,pattern.bytes.size(),&current)) { bool match = true; for (std::size_t i = 0; i < current.size(); ++i) if ((current[i] & pattern.masks[i]) != (pattern.bytes[i] & pattern.masks[i])) match = false; if (match && !cancelled_) { if (snapshot->Replace(cursor,pattern.bytes.size(),replacement)) { result.document = std::move(snapshot); result.count = 1; result.found = cursor; result.search->length = replacement.size(); } } } }
        if (!result.document && request.action <= SearchAction::Replace) {
            auto start = (std::clamp)(cursor,begin,end); if (repeated && !request.previous) start = (std::min<std::uint64_t>)(end,start + pattern.bytes.size());
            const auto found = [&](std::uint64_t at) { result.found = at; return request.previous; };
            ScanBytePattern(*snapshot,pattern,request.previous ? begin : start,request.previous ? start : end,cancelled_,found,progress,&result.error);
            if (result.found == UINT64_MAX && request.wrap && !cancelled_ && result.error.empty()) { result.wrapped = true; ScanBytePattern(*snapshot,pattern,request.previous ? start : begin,request.previous ? end : start,cancelled_,found,progress,&result.error); }
            result.search->length = pattern.bytes.size();
        } else if (!result.document) {
            ScanBytePattern(*snapshot,pattern,begin,end,cancelled_,collect,progress,&result.error);
            if (request.action == SearchAction::ReplaceAll && !offsets.empty() && !cancelled_ && result.error.empty()) { std::vector<std::uint8_t> before,after; if (!snapshot->Read(begin,static_cast<std::size_t>(end - begin),&before)) result.error = L"Could not read replacement range.";
                std::uint64_t copied = 0; for (auto at : offsets) { if (cancelled_ || !result.error.empty()) break; const auto index = at - begin; if (after.size() + index - copied + replacement.size() > 64ULL * 1024 * 1024) { result.error = L"Replacement output exceeds 64 MiB."; break; } after.insert(after.end(),before.begin() + copied,before.begin() + index); after.insert(after.end(),replacement.begin(),replacement.end()); copied = index + pattern.bytes.size(); }
                if (result.error.empty() && !cancelled_) { if (after.size() + before.size() - copied > 64ULL * 1024 * 1024) result.error = L"Replacement output exceeds 64 MiB."; else { after.insert(after.end(),before.begin() + copied,before.end()); if (!snapshot->Replace(begin,end - begin,after)) result.error = L"Could not apply the replacement plan."; else result.document = std::move(snapshot); } }
            }
        }
        return result;
    }); } catch (...) { status_.SetPaneText(0,L"Could not start pattern search."); }
}
void CMainFrame::FinishByteSearch(EditorJobResult result) {
    auto* view = byte_search_view_; if (!view || cancelled_ || revision_ != job_revision_) { status_.SetPaneText(0,L"Search cancelled or input changed; old results discarded."); return; }
    if (!result.error.empty()) { status_.SetPaneText(0,result.error.c_str()); return; }
    if (result.document) { const auto before_size = document_.Size(), after_size = result.document->Size(); document_.Swap(*result.document); DocumentChanged(); hex_.SetDocument(&document_); ShowPage(0); if (last_search_.selection_only) { if (after_size >= before_size) byte_scope_end_ += after_size - before_size; else byte_scope_end_ -= before_size - after_size; } }
    if (result.found != UINT64_MAX) { view->SelectRange(result.found,result.search->length); byte_last_match_ = result.found; byte_last_length_ = result.search->length; byte_scope_revision_ = revision_; }
    const auto message = result.found != UINT64_MAX && !result.document ? L"Match at decimal byte offset " + std::to_wstring(result.found) : std::to_wstring(result.count) + (result.document ? L" replacement(s). One Undo restores the operation." : L" match(es)."); status_.SetPaneText(0,(message + (result.wrapped ? L" Wrapped." : L"")).c_str());
    if (last_search_.action == SearchAction::FindAll && result.search) { const auto generation = revision_; search_results_.navigate = [this,view,generation](const NativeSearchMatch& match) { if (revision_ != generation) { status_.SetPaneText(0,L"Search results are stale."); return; } view->SelectRange(match.offset,match.length); view->SetFocus(); }; search_results_.Show(this,std::move(result.search->matches),message + (result.count > 100000 ? L" First 100,000 shown." : L" Double-click to navigate.")); }
}

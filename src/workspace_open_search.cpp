#include "workspace_frame.h"
#include "main_frame.h"
#include "resource.h"
#include "ui/feature_dialogs.h"
#include <algorithm>
#include <chrono>

void CWorkspaceFrame::SearchStatus(const std::wstring& text) {
    if (scoped_search_.GetSafeHwnd()) scoped_search_.SetDlgItemTextW(IDC_SEARCH_STATUS,text.c_str());
    if (active_session_ >= 0) sessions_[active_session_]->status_.SetPaneText(0,text.c_str());
}

void CWorkspaceFrame::ShowFindReplace(bool replace) {
    if (active_session_ < 0) return;
    scoped_search_.allow_all_open = true;
    scoped_search_.cancel = [this]() { open_search_cancel_ = true; };
    scoped_search_.execute = [this](const SearchRequest& request) {
        if (request.all_open) { ExecuteOpenSearch(request); return; }
        if (open_search_io_.valid() || open_search_preview_) { SearchStatus(L"Wait for the open-document search, or close this dialog to cancel it."); return; }
        if (active_session_ < 0) { SearchStatus(L"Open a document first."); return; }
        if (sessions_[active_session_]->IsBusy()) { SearchStatus(L"The current editor is busy. Wait for its operation to finish."); return; }
        SearchStatus(L"Current document: results appear in the editor status bar.");
        sessions_[active_session_]->ExecuteSearch(request);
    };
    auto* frame = sessions_[active_session_];
    frame->ActiveEditor();
    scoped_search_.Show(replace,!frame->AdvancedActive() && frame->active_tab_ != 1,this);
}

void CWorkspaceFrame::ExecuteOpenSearch(const SearchRequest& request) {
    if (open_search_io_.valid() || open_search_preview_) { SearchStatus(L"Search is running. Close this dialog or press Esc to cancel."); return; }
    if (request.find.IsEmpty()) { SearchStatus(L"Enter text to find."); return; }
    if (request.hex || (request.mode != 0 && request.mode != 2 && request.mode != 3)) { SearchStatus(L"All open documents supports Text, Text escapes and Regex. Use Current document for Hex."); return; }
    if (request.action == SearchAction::FindAll && multi_search_.Busy()) { SearchStatus(L"Wait for the existing files/folder search before showing another result list."); return; }
    const bool mutation = request.action == SearchAction::Replace || request.action == SearchAction::ReplaceAll;
    OpenSearchJob job; job.request = request; std::size_t total = 0,origin = SIZE_MAX;
    std::vector<std::wstring> paths;
    for (std::size_t i = 0; i < sessions_.size(); ++i) {
        auto* frame = sessions_[i]; if (frame->comparison_session_ || !frame->document_.IsOpen()) continue;
        if (frame->IsBusy() || DocumentLocked(frame->document_)) { SearchStatus(L"An open editor is busy. Wait or cancel its operation first."); return; }
        if (frame->text_stale_ && frame->document_.Size() <= 32 * 1024 * 1024) frame->RefreshText();
        if (frame->text_stale_ || !frame->text_editable_) { ++job.skipped; continue; }
        if (mutation && frame->user_read_only_) { SearchStatus(L"An open text document is read-only. No replacements were made; choose Current document or turn off read-only first."); return; }
        OpenSearchItem item; item.session = frame->workspace_session_id_; item.revision = frame->revision_; item.generation = frame->text_.Generation();
        item.path = frame->untitled_ ? frame->untitled_name_ : frame->document_.Path(); item.input = frame->text_.Utf8(); total += item.input.size();
        if (total > 128 * 1024 * 1024) { SearchStatus(L"Open text snapshots exceed 128 MiB. Close some documents or search Current document."); return; }
        auto* editor = frame->ActiveEditor(); if (!editor) editor = &frame->text_;
        item.start = editor->Call(SCI_GETSELECTIONSTART); item.end = editor->Call(SCI_GETSELECTIONEND);
        if (static_cast<int>(i) == active_session_) origin = job.items.size();
        paths.push_back(frame->document_.Path()); job.items.push_back(std::move(item));
    }
    if (job.items.empty()) { SearchStatus(L"No editable text documents are open. Binary and oversized files use Current document / Hex."); return; }
    if (origin == SIZE_MAX) { origin = request.previous ? job.items.size() - 1 : 0; job.items[origin].start = job.items[origin].end = request.previous ? job.items[origin].input.size() : 0; }
    if (mutation && !LockPaths(paths,true,&open_search_io_)) { SearchStatus(L"Could not lock the participating documents; no replacements were made."); return; }
    open_search_cancel_ = false; for (const auto& item : job.items) open_search_sessions_.insert(item.session);
    for (auto* frame : sessions_) if (open_search_sessions_.count(frame->workspace_session_id_)) frame->last_search_ = request;
    try {
        open_search_io_ = std::async(std::launch::async,[this,job = std::move(job),origin]() mutable {
            const auto& request = job.request; NativeSearchOptions options; options.find = request.find.GetString(); options.replacement = request.replacement.GetString();
            options.action = static_cast<int>(request.action); options.syntax = request.mode == 3 ? 2 : request.mode == 2 ? 1 : 0;
            options.match_case = request.match_case; options.whole_word = request.whole_word; options.previous = request.previous; options.wrap = false;
            const auto plan = [&](OpenSearchItem& item,std::uint64_t begin,std::uint64_t end,std::uint64_t first,std::uint64_t last,NativeSearchOptions operation) {
                return PlanEditorSearch(item.input,operation,begin,end,first,last,open_search_cancel_,{},&item.plan,&job.error);
            };
            if (request.action <= SearchAction::Replace) {
                const auto count = job.items.size();
                for (std::size_t step = 0; step <= count && !open_search_cancel_; ++step) {
                    if (!request.wrap && (request.previous ? step > origin : step >= count - origin)) break;
                    const auto index = request.previous ? (origin + count - step % count) % count : (origin + step) % count;
                    auto& item = job.items[index]; const bool tail = step == count;
                    if (tail && !request.wrap) break;
                    const auto start = step == 0 ? item.start : request.previous ? item.input.size() : 0;
                    const auto end = step == 0 ? item.end : start;
                    auto operation = options; if (step) operation.action = 0;
                    if (!plan(item,tail && request.previous ? item.start : 0,tail && !request.previous ? item.end : item.input.size(),start,end,operation)) break;
                    if (item.plan.found || item.plan.changed) { if (item.plan.output.size() > 32 * 1024 * 1024) { job.error = L"Replacement exceeds the 32 MiB text editor limit. No documents were changed."; break; } job.chosen = index; job.count = item.plan.count; job.wrapped = request.previous ? step > origin : step >= count - origin; break; }
                }
            } else {
                std::size_t output_size = 0,match_count = 0;
                for (auto& item : job.items) {
                    if (!plan(item,0,item.input.size(),0,0,options)) break;
                    job.count += item.plan.count; output_size += item.plan.changed ? item.plan.output.size() : item.input.size();
                    if (item.plan.output.size() > 32 * 1024 * 1024 || output_size > 128 * 1024 * 1024) { job.error = L"Replacement exceeds the text editor memory limit. No documents were changed."; break; }
                    if (request.action == SearchAction::FindAll) { const auto keep = (std::min)(item.plan.matches.size(),100000 - match_count); item.plan.matches.resize(keep); match_count += keep; }
                }
            }
            if (open_search_cancel_) job.error = L"Search cancelled; documents unchanged.";
            return std::move(job);
        });
    } catch (...) { open_search_sessions_.clear(); LockPaths({},false,&open_search_io_); SearchStatus(L"Could not start the search worker; documents unchanged."); return; }
    SearchStatus(L"Searching open document snapshots... Close this dialog or press Esc to cancel."); SetTimer(5,30,nullptr);
}

void CWorkspaceFrame::TickOpenSearch() {
    if (!open_search_io_.valid() || open_search_io_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) return;
    KillTimer(5); OpenSearchJob job;
    try { job = open_search_io_.get(); } catch (...) { job.error = L"Open-document search failed; documents unchanged."; }
    const auto find = [&](std::uint64_t id) -> CMainFrame* { for (auto* frame : sessions_) if (frame->workspace_session_id_ == id) return frame; return nullptr; };
    if (open_search_cancel_) job.error = L"Search cancelled; documents unchanged.";
    for (const auto& item : job.items) {
        auto* frame = find(item.session);
        if (!frame || frame->revision_ != item.revision || frame->text_.Generation() != item.generation || frame->text_stale_ || frame->IsBusy()) { job.error = L"An open document changed while searching. Run it again; no replacements were applied."; break; }
        if (item.plan.changed && frame->user_read_only_) { job.error = L"A document became read-only; no replacements were applied."; break; }
    }
    if (job.error.empty() && job.request.preview) {
        std::vector<std::vector<std::wstring>> rows; std::vector<std::size_t> indexes;
        for (std::size_t i = 0; i < job.items.size(); ++i) if (job.items[i].plan.changed) {
            const auto& item = job.items[i]; indexes.push_back(i); rows.push_back({item.path,std::to_wstring(item.plan.count),std::to_wstring(item.input.size()) + L" / " + std::to_wstring(item.plan.output.size())});
        }
        if (!rows.empty()) {
            CReviewList preview(this,L"Preview replacements - select files, then Apply selected",{L"Open document",L"Replacements",L"Before / after UTF-8 bytes"},std::move(rows)); preview.accept_label = L"Apply selected";
            preview.details = [&](std::size_t row) {
                const auto& item = job.items[indexes[row]];
                const auto excerpt = [](const std::string& value) { auto end = (std::min<std::size_t>)(value.size(),32768); while (end && end < value.size() && (static_cast<unsigned char>(value[end]) & 0xC0) == 0x80) --end; return CCodeEditor::FromUtf8(value.substr(0,end)) + (end < value.size() ? L"\r\n[Preview limited to the first 32 KiB; full replacement is retained.]" : L""); };
                CTextResultDialog details(&preview,L"Before / after replacement preview",L"BEFORE\r\n" + excerpt(item.input) + L"\r\n\r\nAFTER\r\n" + excerpt(item.plan.output)); details.DoModal();
            };
            open_search_preview_ = true; INT_PTR answer = IDCANCEL;
            try { answer = preview.DoModal(); } catch (...) { job.error = L"Could not display replacement preview; documents unchanged."; }
            open_search_preview_ = false;
            if (answer != IDOK || open_search_cancel_) job.error = L"Replacement preview cancelled; documents unchanged.";
            std::set<std::size_t> accepted; for (auto index : preview.selected) if (index < indexes.size()) accepted.insert(indexes[index]);
            job.count = 0;
            for (std::size_t i = 0; i < job.items.size(); ++i) { if (!accepted.count(i)) job.items[i].plan.changed = false; else job.count += job.items[i].plan.count; }
            for (const auto& item : job.items) if (item.plan.changed) { auto* frame = find(item.session); if (!frame || frame->revision_ != item.revision || frame->text_.Generation() != item.generation || frame->text_stale_ || frame->IsBusy() || frame->user_read_only_) { job.error = L"A selected document changed during preview; no replacements were applied."; break; } }
        }
    }
    // Release only this job's locks after the final generation check, on the UI thread.
    LockPaths({},false,&open_search_io_); open_search_sessions_.clear();
    if (!job.error.empty()) { SearchStatus(job.error); return; }
    std::size_t changed_files = 0;
    for (auto& item : job.items) if (item.plan.changed) {
        auto* frame = find(item.session); frame->text_.ReplaceGenerated(CCodeEditor::FromUtf8(item.plan.output),0);
        frame->SendMessage(WM_COMMAND,MAKEWPARAM(frame->text_.GetDlgCtrlID(),EN_CHANGE),reinterpret_cast<LPARAM>(frame->text_.GetSafeHwnd())); ++changed_files;
    }
    if (job.chosen != SIZE_MAX) {
        auto& item = job.items[job.chosen]; auto* frame = find(item.session);
        ActivateSession(static_cast<int>(std::find(sessions_.begin(),sessions_.end(),frame) - sessions_.begin())); frame->ShowPage(1);
        auto* editor = frame->ActiveEditor(); if (!editor) editor = &frame->text_;
        editor->Call(SCI_SETSEL,item.plan.cursor,item.plan.cursor + item.plan.length); editor->Call(SCI_SCROLLCARET); editor->SetFocus();
    }
    std::wstring status = std::to_wstring(job.count) + (changed_files ? L" replacement(s) in " + std::to_wstring(changed_files) + L" document(s). Undo is available in each tab; files are not saved." : L" match(es) in open text documents.");
    if (job.wrapped) status += L" Wrapped."; if (job.skipped) status += L" Skipped " + std::to_wstring(job.skipped) + L" binary/oversized document(s).";
    SearchStatus(status);
    if (job.request.action == SearchAction::FindAll) {
        std::vector<simple_editor::services::MultiSearchMatch> matches;
        for (auto& item : job.items) for (auto& match : item.plan.matches) {
            simple_editor::services::MultiSearchMatch result; result.path = item.path; result.session = item.session; result.revision = item.revision; result.generation = item.generation; result.native_text = true; result.match = std::move(match); matches.push_back(std::move(result));
        }
        if (job.count > matches.size()) status += L" First 100,000 results shown.";
        multi_search_.capture = [this](int mode,std::vector<simple_editor::services::MultiSearchSource>* sources,std::wstring* error) { return CaptureSearchSources(mode,sources,error); };
        multi_search_.navigate = [this](const simple_editor::services::MultiSearchMatch& match) { NavigateSearchResult(match); };
        if (!multi_search_.Present(this,std::move(matches),status)) SearchStatus(status + L" Result list is busy; repeat Find All after it finishes.");
    }
}

void CWorkspaceFrame::StopOpenSearch() {
    open_search_cancel_ = true; if (open_search_io_.valid()) { try { open_search_io_.get(); } catch (...) {} }
    open_search_sessions_.clear(); LockPaths({},false,&open_search_io_);
}

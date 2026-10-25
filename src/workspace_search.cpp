#include "workspace_frame.h"
#include "main_frame.h"
#include "resource.h"
#include <algorithm>
using namespace simple_editor::core;
using namespace simple_editor::services;
void CWorkspaceFrame::OnSearchFiles() {
    multi_search_.capture = [this](int mode,std::vector<MultiSearchSource>* sources,std::wstring* error) { return CaptureSearchSources(mode,sources,error); };
    multi_search_.navigate = [this](const MultiSearchMatch& match) { NavigateSearchResult(match); }; multi_search_.Show(this);
}
bool CWorkspaceFrame::CaptureSearchSources(int mode,std::vector<MultiSearchSource>* sources,std::wstring* error) {
    std::vector<MultiSearchSource> result; std::size_t total = 0;
    for (auto* frame : sessions_) {
        if (frame->comparison_session_ || !frame->document_.IsOpen()) continue;
        MultiSearchSource source; source.path = frame->untitled_ ? frame->untitled_name_ : frame->document_.Path(); source.session = frame->workspace_session_id_; source.revision = frame->revision_; source.generation = frame->text_.Generation();
        if (mode < 3 && !frame->text_stale_ && frame->text_editable_) {
            source.native_text = true; source.utf8 = frame->text_.Utf8(); total += source.utf8.size();
            if (total > 128 * 1024 * 1024) { if (error) *error = L"Open-file text snapshots exceed 128 MiB. Search fewer documents or a disk folder."; return false; }
        } else { source.document = SnapshotForPath(source.path,error); if (!source.document) return false; }
        result.push_back(std::move(source));
    }
    if (result.empty()) { if (error) *error = L"Open file editors before searching all open files."; return false; }
    *sources = std::move(result); return true;
}
void CWorkspaceFrame::NavigateSearchResult(const MultiSearchMatch& match) {
    const auto status = [&](const wchar_t* text) { if (multi_search_.GetSafeHwnd()) multi_search_.SetDlgItemTextW(1914,text); };
    CMainFrame* frame = nullptr;
    if (match.session) {
        for (auto* candidate : sessions_) if (candidate->workspace_session_id_ == match.session) { frame = candidate; break; }
        if (!frame || frame->revision_ != match.revision || (match.native_text && frame->text_.Generation() != match.generation)) { status(L"This result is stale or its tab was closed. Run the search again."); return; }
    } else {
        FileStamp now; std::wstring error; if (!InspectFile(match.path,&now,&error) || !SameFileState(now,match.stamp)) { status(L"The disk file changed after the search. Run the search again."); return; }
        for (auto* candidate : sessions_) if (!candidate->comparison_session_ && CompareStringOrdinal(candidate->document_.Path().c_str(),-1,match.path.c_str(),-1,TRUE) == CSTR_EQUAL) {
            if (candidate->document_.IsModified() || candidate->text_dirty_ || candidate->text_.GetModify() || !candidate->document_.ValidateSource(true,&error)) { status(L"An existing editor has a draft or an older disk version. Its content was preserved; search the open editors instead."); return; } frame = candidate; break;
        }
        if (!frame) { if (!OpenPath(match.path,true)) { status(L"Could not open the searched file."); return; } frame = sessions_[active_session_]; }
    }
    if (!match.session) {
        FileStamp now; std::wstring error;
        if (!InspectFile(match.path,&now,&error) || !SameFileState(now,match.stamp) || frame->document_.SourceIdentity() != std::make_pair(match.stamp.volume,match.stamp.identity) || !frame->document_.ValidateSource(true,&error)) { status(L"The disk version changed while opening the result. Its current tab was retained without applying the old selection."); return; }
    }
    if (frame->IsBusy()) { status(L"The target editor is busy; wait before navigating."); return; }
    const auto found = std::find(sessions_.begin(),sessions_.end(),frame); if (found == sessions_.end()) return; ActivateSession(static_cast<int>(found - sessions_.begin()));
    if (match.binary) { frame->ShowPage(0); frame->hex_.SelectRange(match.match.offset,match.match.length); frame->hex_.SetFocus(); }
    else {
        if (!match.native_text) { frame->requested_encoding_ = match.encoding; frame->text_stale_ = true; }
        frame->ShowPage(1); if (!frame->text_editable_) { status(L"The searched text could not be decoded in this editor."); return; }
        frame->text_.Call(SCI_SETSEL,match.match.offset,match.match.offset + match.match.length); frame->text_.Call(SCI_SCROLLCARET); frame->text_.SetFocus();
    }
}

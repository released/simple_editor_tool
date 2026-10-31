#include "workspace_frame.h"
#include "main_frame.h"
#include <algorithm>

using namespace simple_editor::core;
namespace {
bool SameSidePath(const std::wstring& a,const std::wstring& b) {
    return CompareStringOrdinal(std::filesystem::absolute(a).lexically_normal().c_str(),-1,
        std::filesystem::absolute(b).lexically_normal().c_str(),-1,TRUE) == CSTR_EQUAL;
}
}
bool CWorkspaceFrame::ApplyComparisonCopy(const TextSideCopy& copy,std::wstring* error) {
    PreparedTextSideCopy prepared; static_cast<TextSideCopy&>(prepared) = copy;
    const std::atomic_bool cancel{false};
    return CaptureComparisonCopy(prepared,error) && prepared.Prepare(cancel,error) && ApplyPreparedComparisonCopy(prepared,error);
}
std::shared_ptr<TextInputSnapshot> CWorkspaceFrame::CaptureTextInput(const std::wstring& path,std::wstring* error,bool include_display) {
    try {
        auto input = std::make_shared<TextInputSnapshot>();
        for (auto* session : sessions_) if (!session->comparison_session_ && session->document_.IsOpen() &&
            (SameSidePath(path,session->document_.Path()) || (session->untitled_ && path == session->untitled_name_))) {
            input->document = session->document_.Snapshot(error); if (!input->document) return nullptr;
            input->session = session->workspace_session_id_; input->revision = session->revision_; input->generation = session->text_.Generation();
            input->loaded = !session->text_stale_ && session->text_editable_; input->requested = session->requested_encoding_; input->codec = session->text_codec_;
            input->dirty = session->text_dirty_ || session->text_.GetModify();
            if ((input->dirty || include_display) && session->text_.Call(SCI_GETLENGTH) > 32 * 1024 * 1024) { if (error) *error = L"Text snapshots accept up to 32 MiB of editor data. The draft is retained."; return nullptr; }
            if (input->dirty || (include_display && input->loaded)) { input->displayed = session->text_.Utf8(); input->original = session->text_original_; }
            return input;
        }
        input->document = std::make_unique<BinaryDocument>(); if (!input->document->Open(path,error)) return nullptr;
        return input;
    } catch (...) { if (error) *error = L"Could not capture the text source. Existing drafts are retained."; return nullptr; }
}
bool CWorkspaceFrame::TextInputCurrent(const TextInputSnapshot& input,const std::wstring& path,std::wstring* error) const {
    bool found = !input.session;
    for (const auto* session : sessions_) if (!session->comparison_session_ && session->document_.IsOpen()) {
        if (session->workspace_session_id_ == input.session) {
            found = session->revision_ == input.revision && session->text_.Generation() == input.generation && session->requested_encoding_ == input.requested;
            break;
        }
        if (!input.session && (SameSidePath(path,session->document_.Path()) || (session->untitled_ && path == session->untitled_name_))) found = false;
    }
    if (!found) { if (error) *error = L"A source or target changed since comparison. Compare again before copying."; return false; }
    return input.document && input.document->ValidateSource(true,error);
}
bool CWorkspaceFrame::CaptureComparisonCopy(PreparedTextSideCopy& copy,std::wstring* error) {
    for (unsigned side = 0; side < 2; ++side) { copy.inputs[side] = CaptureTextInput(side ? copy.right_path : copy.left_path,error,true); if (!copy.inputs[side]) return false; }
    return true;
}
bool CWorkspaceFrame::ApplyPreparedComparisonCopy(const PreparedTextSideCopy& copy,std::wstring* error) {
    const auto fail = [&](const wchar_t* message) { if (error) *error = message; return false; };
    if (!copy.prepared) return fail(L"Could not prepare the side copy. No content was copied.");
    const auto& target_path = copy.to_right ? copy.right_path : copy.left_path;
    const auto& expected = copy.to_right ? copy.right : copy.left;
    if (target_path.empty() || copy.left_path.empty() || copy.right_path.empty()) return fail(L"Select both file paths and compare again before copying.");
    CMainFrame* target = nullptr;
    for (auto* session : sessions_) if (!session->comparison_session_ && session->document_.IsOpen() &&
        (SameSidePath(session->document_.Path(),target_path) || (session->untitled_ && session->untitled_name_ == target_path))) { target = session; break; }
    if (PathLocked(target_path) || (target && (target->IsBusy() || target->MutationBlocked()))) return fail(L"The target is read-only or busy. No content was copied.");
    for (unsigned side = 0; side < 2; ++side) {
        const auto& path = side ? copy.right_path : copy.left_path;
        if (!copy.inputs[side] || !TextInputCurrent(*copy.inputs[side],path,error)) return false;
    }
    if (copy.replacement == expected) return true;
    const auto input = copy.inputs[copy.to_right ? 1 : 0];
    if (!target) {
        // Create an inactive normal editor so its existing Save/close/recovery policy owns the draft.
        target = CreateSession(false); if (!target) return fail(L"Could not create a target editor. No content was copied.");
        target->document_.Swap(*input->document); target->hex_.SetDocument(&target->document_);
        target->SetWindowTextW((target->document_.Path() + L" - Simple Editor Tool").c_str());
    }
    if (!input->session || !input->loaded) {
        target->requested_encoding_ = copy.target_codec.encoding; target->text_codec_ = copy.target_codec; target->text_original_ = copy.initial;
        target->text_.SetWindowTextW(TextForEditControl(copy.initial).c_str()); target->text_.SetModify(FALSE);
        target->text_editable_ = true; target->text_stale_ = target->text_dirty_ = false; target->ConfigureFileSyntax();
    }
    target->SetSaveReadOnly(DocumentLocked(target->document_));
    CancelCloseAll(); target->ShowPage(1);
    target->text_.ReplaceGenerated(copy.displayed,0);
    target->OnTextChanged(); target->RequestIndentation(true); RefreshLabels();
    return true;
}
bool CWorkspaceFrame::ComparisonSideAction(const std::wstring& path,int action) {
    for (auto* target : sessions_) {
        if (target->comparison_session_ || !target->document_.IsOpen() ||
            (!SameSidePath(target->document_.Path(),path) && !(target->untitled_ && target->untitled_name_ == path))) continue;
        if (target->IsBusy() || target->MutationBlocked()) return false;
        if (action == 0) return target->SaveCurrent();
        if (target->text_stale_ || !target->text_editable_) return false;
        if (action == 1 ? !target->text_.CanUndo() : !target->text_.CanRedo()) return false;
        if (action == 1) target->text_.Undo(); else target->text_.Redo();
        return true;
    }
    return false;
}

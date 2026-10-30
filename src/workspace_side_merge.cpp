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
    const auto fail = [&](const wchar_t* message) { if (error) *error = message; return false; };
    const auto& target_path = copy.to_right ? copy.right_path : copy.left_path;
    const auto& expected = copy.to_right ? copy.right : copy.left;
    if (target_path.empty() || copy.left_path.empty() || copy.right_path.empty()) return fail(L"Select both file paths and compare again before copying.");
    CMainFrame* target = nullptr;
    for (auto* session : sessions_) if (!session->comparison_session_ && session->document_.IsOpen() &&
        (SameSidePath(session->document_.Path(),target_path) || (session->untitled_ && session->untitled_name_ == target_path))) { target = session; break; }
    if (PathLocked(target_path) || (target && (target->IsBusy() || target->MutationBlocked()))) return fail(L"The target is read-only or busy. No content was copied.");
    std::unique_ptr<BinaryDocument> target_snapshot;
    for (unsigned side = 0; side < 2; ++side) {
        const auto& path = side ? copy.right_path : copy.left_path;
        auto snapshot = SnapshotForPath(path,error); if (!snapshot) return false;
        std::vector<std::uint8_t> bytes; std::wstring raw; TextCodec codec;
        if (snapshot->Size() > 16 * 1024 * 1024 || !snapshot->Read(0,static_cast<std::size_t>(snapshot->Size()),&bytes) ||
            !snapshot->ValidateSource(true,error) || !DecodeText(bytes,(side ? copy.right_codec : copy.left_codec).encoding,&raw,&codec,error)) return fail(L"Could not validate the comparison sources. No content was copied.");
        if (raw != (side ? copy.right : copy.left)) return fail(L"A source or target changed since comparison. Compare again before copying.");
        if (static_cast<bool>(side) == copy.to_right) target_snapshot = std::move(snapshot);
    }
    if (target) {
        if (target->text_stale_) target->RefreshText();
        CString value; target->text_.GetWindowTextW(value);
        if (!target->text_editable_ || TextFromEditControl(target->text_original_,std::wstring(value.GetString(),value.GetLength())) != expected)
            return fail(L"The target editor uses different text decoding. Select matching encodings and compare again.");
    }
    const auto codec = target ? target->text_codec_ : copy.to_right ? copy.right_codec : copy.left_codec;
    const auto displayed = TextForEditControl(copy.replacement);
    std::vector<std::uint8_t> encoded;
    if (!EncodeText(TextFromEditControl(expected,displayed),codec,true,&encoded,error)) return false;
    if (encoded.size() > 16 * 1024 * 1024) return fail(L"The copied text exceeds the 16 MiB comparison input limit. No content was copied.");
    if (copy.replacement == expected) return true;
    if (!target) {
        // Create an inactive normal editor so its existing Save/close/recovery policy owns the draft.
        target = CreateSession(false); if (!target) return fail(L"Could not create a target editor. No content was copied.");
        target->document_.Swap(*target_snapshot); target->hex_.SetDocument(&target->document_);
        target->requested_encoding_ = codec.encoding; target->text_stale_ = true; target->RefreshText();
        target->SetWindowTextW((target->document_.Path() + L" - Simple Editor Tool").c_str());
        target->SetSaveReadOnly(DocumentLocked(target->document_));
    }
    CancelCloseAll(); target->ShowPage(1);
    target->text_.ReplaceGenerated(displayed,0);
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

#include "main_frame.h"
#include "resource.h"
#include "ui/feature_dialogs.h"
#include "core/line_merge.h"
#include "core/pattern_search.h"
using namespace simple_editor::core;
void CMainFrame::OnFeature(UINT command) {
    if (IsBusy()) return;
    if (command == ID_FEATURE_BOOKMARK_ADD || command == ID_FEATURE_BOOKMARK_LIST) BookmarkCommand(command == ID_FEATURE_BOOKMARK_ADD);
    else if (command == ID_FEATURE_TABLE_RULES || command == ID_FEATURE_TEXT_RULES || command == ID_FEATURE_ANCHORS || command == ID_FEATURE_RENAMES) { if (AdvancedActive() && EnsureAdvanced()) advanced_.FeatureSettings(command); else status_.SetPaneText(0,L"Open the relevant comparison page first."); }
    else if (command == ID_FEATURE_INSPECTOR) ShowLiveInspector();
    else if ((command >= ID_FEATURE_SORT_ASC && command <= ID_FEATURE_TRIM_END) || command == ID_FEATURE_COMPLETE) StartTextCommand(command);
    else if (command == ID_FEATURE_CLONE) ToggleTextClone();
    else if (command == ID_FEATURE_CRC) CustomCrcDialog();
    else if (command == ID_FEATURE_TAIL) { if (untitled_ || !document_.IsOpen() || comparison_session_) status_.SetPaneText(0,L"Open a saved file in an editor tab to follow its disk log."); else log_tail_.Show(this,document_.Path(),requested_encoding_); }
}
void CMainFrame::ShowLiveInspector() {
    live_inspector_.inspect = [this](bool little) { LiveInspection result; result.revision = revision_; auto* view = ActiveHex(); if (!view || IsBusy()) { result.error = L"Choose an idle Hex view to inspect bytes."; return result; } result.offset = view->CursorOffset(); auto* document = view == &hex_ ? &document_ : view == &compare_left_ ? &compare_left_document_ : &compare_right_document_; result.writable = view == &hex_ && !MutationBlocked() && !view->ReadOnly(); InspectNumbers(*document,result.offset,little,&result.values,&result.error); return result; };
    live_inspector_.write = [this](const LiveInspection& source,const std::wstring& type,const std::wstring& value,bool little,std::wstring* error) { if (IsBusy() || MutationBlocked() || hex_.ReadOnly() || ActiveHex() != &hex_ || source.revision != revision_ || source.offset != hex_.CursorOffset() || !source.writable || !document_.ValidateSource(true,error)) { if (error->empty()) *error = L"The selected field is stale, read-only or busy. Select it again."; return false; } std::wstring token; if (type.find(L"uint") == 0) token = L"u" + type.substr(4); else if (type.find(L"int") == 0) token = L"i" + type.substr(3); else if (type.find(L"float") == 0) token = L"f" + type.substr(5); else return false; BytePattern bytes; if (!ParseBytePattern(token + (little ? L"le:" : L"be:") + value,true,&bytes,error) || bytes.bytes.size() > document_.Size() - (std::min)(source.offset,document_.Size()) || !document_.Replace(source.offset,bytes.bytes.size(),bytes.bytes)) return false; DocumentChanged(); hex_.Invalidate(); return true; };
    live_inspector_.Show(this);
}
void CMainFrame::BookmarkCommand(bool add) {
    if (!document_.IsOpen() || comparison_session_ || !ApplyTextEdits()) return;
    if (add) {
        NamedHexRange range; range.offset = hex_.SelectionLength() ? hex_.SelectionStart() : hex_.CursorOffset(); range.length = hex_.SelectionLength() ? hex_.SelectionLength() : range.offset < document_.Size() ? 1 : 0;
        CFeatureInput input(this,L"Name a Hex range",L"First line: name (up to 128 characters). Remaining lines: comment (up to 512). Select bytes in Hex to mark a larger range.",L"Range"); input.validate = [&](const std::wstring& text,std::wstring* error) { const auto newline = text.find_first_of(L"\r\n"); range.name = text.substr(0,newline); range.comment = newline == std::wstring::npos ? L"" : text.substr(text.find_first_not_of(L"\r\n",newline) == std::wstring::npos ? text.size() : text.find_first_not_of(L"\r\n",newline)); HexBookmarks check; return check.Add(range,document_.Size(),error); };
        path_dialog_ = true; const auto answer = input.DoModal(); path_dialog_ = false; if (answer != IDOK) return; CColorDialog color(range.color,CC_FULLOPEN,this); path_dialog_ = true; const auto chosen = color.DoModal(); path_dialog_ = false; if (chosen != IDOK) return; range.color = color.GetColor();
        if (!bookmarks_.Matches(document_)) for (auto& previous : bookmarks_.ranges) previous.stale = true; if (!InspectFile(document_.Path(),&bookmarks_.source,nullptr)) return; bookmarks_.stale = false; std::wstring error; if (!bookmarks_.Add(range,document_.Size(),&error)) status_.SetPaneText(0,error.c_str()); hex_.Invalidate(); return;
    }
    const bool valid = bookmarks_.Matches(document_); std::vector<std::vector<std::wstring>> rows; for (const auto& range : bookmarks_.ranges) { wchar_t offset[32]{}; swprintf_s(offset,L"0x%llX",range.offset); rows.push_back({range.name,offset,std::to_wstring(range.length),valid && !range.stale ? L"Current" : L"Stale - recreate range",range.comment}); }
    CReviewList dialog(this,L"Hex range bookmarks",{L"Name",L"Physical offset",L"Bytes",L"State",L"Comment"},std::move(rows)); dialog.remove = [&](std::size_t index) { bookmarks_.ranges.erase(bookmarks_.ranges.begin() + index); hex_.Invalidate(); return true; }; path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false;
    if (answer == IDOK && !dialog.selected.empty()) { const auto& range = bookmarks_.ranges[dialog.selected[0]]; if (!range.stale && bookmarks_.Matches(document_)) { ShowPage(0); hex_.SelectRange(range.offset,range.length); hex_.SetFocus(); } else status_.SetPaneText(0,L"This bookmark belongs to an older document version. Recreate its range before navigating."); }
}

#include "advanced_compare.h"
#include "feature_dialogs.h"
#include "../core/source_alignment.h"
#include <algorithm>
using namespace simple_editor::core;

bool CAdvancedCompareView::CreateSourceEditors() {
    const DWORD style = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_HSCROLL | WS_VSCROLL;
    if (!source_left_.Create(style,CRect(),this,588) || !source_right_.Create(style,CRect(),this,589) || !source_base_.Create(style,CRect(),this,592)) return false;
    source_left_label_.Create(L"Left (read-only; character detail: first 8192 chars/line)",WS_CHILD,CRect(),this,590);
    source_right_label_.Create(L"Right (read-only; character detail: first 8192 chars/line)",WS_CHILD,CRect(),this,591);
    source_base_label_.Create(L"Base (read-only; changed lines)",WS_CHILD,CRect(),this,593);
    for (auto* editor : {&source_left_,&source_right_,&source_base_}) {
        editor->SetFont(&content_font_); editor->SetReadOnly(TRUE); editor->SetTheme(theme_); editor->SetWrap(false);
        editor->SetSourcePresentation(true); editor->Call(SCI_SETENDATLASTLINE,0);
        editor->wheel_zoom = [this](short delta) { if (wheel_zoom) wheel_zoom(delta); };
    }
    source_left_label_.SetFont(&ui_font_); source_right_label_.SetFont(&ui_font_); source_base_label_.SetFont(&ui_font_);
    source_left_.selection_changed = [this]() { SourceChanged(1); };
    source_right_.selection_changed = [this]() { SourceChanged(2); }; source_base_.selection_changed = [this]() { SourceChanged(0); }; return true;
}
CCodeEditor* CAdvancedCompareView::SearchEditor() {
    for (CCodeEditor* editor : {static_cast<CCodeEditor*>(&source_left_),static_cast<CCodeEditor*>(&source_right_),static_cast<CCodeEditor*>(&source_base_),static_cast<CCodeEditor*>(&input_left_),static_cast<CCodeEditor*>(&input_right_),static_cast<CCodeEditor*>(&output_)})
        if (editor->GetSafeHwnd() && (editor->GetStyle() & WS_VISIBLE) && ::GetFocus() == editor->GetSafeHwnd()) { review_focus_ = editor; return editor; }
    if (review_focus_ && review_focus_->GetSafeHwnd() && (review_focus_->GetStyle() & WS_VISIBLE)) return review_focus_;
    return SideBySide() && !show_output_ ? &source_left_ : OutputEditor();
}
void CAdvancedCompareView::RefreshSources() {
    if (!source_left_.GetSafeHwnd()) return;
    review_focus_ = nullptr;
    source_sync_ = true; source_aligned_ = false; source_top_[0] = source_top_[1] = source_top_[2] = -1;
    const auto fill = [&](CCodeEditor& editor,const std::vector<TextLine>& lines,const std::wstring& path) {
        std::wstring text; for (const auto& line : lines) { text += line.text; text += line.ending; }
        editor.SetReadOnly(FALSE); editor.SetWindowTextW(TextForEditControl(text).c_str()); editor.SetModify(FALSE); editor.ConfigureLanguage(path);
        editor.Call(SCI_ANNOTATIONCLEARALL);
        for (unsigned indicator : {11u,12u}) { editor.Call(SCI_SETINDICATORCURRENT,indicator); editor.Call(SCI_INDICATORCLEARRANGE,0,editor.Call(SCI_GETLENGTH)); }
        editor.Call(SCI_MARKERDELETEALL,26); editor.Call(SCI_MARKERDEFINE,26,SC_MARK_BACKGROUND); editor.Call(SCI_MARKERSETBACK,26,RGB(50,160,185)); editor.Call(SCI_MARKERSETALPHA,26,75);
        editor.Call(SCI_MARKERDELETEALL,25); editor.Call(SCI_MARKERDEFINE,25,SC_MARK_BACKGROUND);
        editor.Call(SCI_MARKERSETBACK,25,theme_ == 1 ? RGB(100,80,25) : RGB(255,235,160)); editor.Call(SCI_MARKERSETALPHA,25,65); editor.SetReadOnly(TRUE);
    };
    const std::vector<TextLine> empty;
    const bool active = model_ && !stale_ && !pasted_inputs_ && mode_ < 4;
    fill(source_left_,active ? model_->left : empty,source_left_path_);
    fill(source_right_,active ? model_->right : empty,source_right_path_);
    fill(source_base_,active && model_->three_way ? model_->base : empty,source_base_path_);
    left_peer_.assign(model_ ? model_->left.size() : 0,-1); right_peer_.assign(model_ ? model_->right.size() : 0,-1);
    if (active) {
        int left = -1,right = -1; std::size_t ordinal = 0;
        for (const auto& row : text_rows_) {
            if (row.left >= 0) left = row.left; if (row.right >= 0) right = row.right;
            if (row.left >= 0) left_peer_[row.left] = right;
            if (row.right >= 0) right_peer_[row.right] = left;
            if (model_->blocks[row.block].changed) { if (row.left >= 0) source_left_.Call(SCI_MARKERADD,source_left_.NativeLine(row.left),25); if (row.right >= 0) source_right_.Call(SCI_MARKERADD,source_right_.NativeLine(row.right),25); if (row.base >= 0 && model_->three_way) source_base_.Call(SCI_MARKERADD,source_base_.NativeLine(row.base),25); }
            if (presentation_ && ordinal < presentation_->characters.size()) {
                const auto& detail = presentation_->characters[ordinal];
                const auto mark = [](CCodeEditor& editor,int line,const std::vector<InlineSpan>& spans,unsigned indicator) { if (line < 0) return; const auto start = editor.Call(SCI_POSITIONFROMLINE,editor.NativeLine(line)); editor.Call(SCI_SETINDICATORCURRENT,indicator); for (const auto& span : spans) { const auto a = editor.Call(SCI_POSITIONRELATIVECODEUNITS,start,span.begin),b = editor.Call(SCI_POSITIONRELATIVECODEUNITS,start,span.begin + span.length); editor.Call(SCI_INDICATORFILLRANGE,a,b - a); } };
                mark(source_left_,row.left,detail.left,11); mark(source_right_,row.right,detail.right,12);
            } ++ordinal;
        }
        const auto leading = [](std::vector<int>& peers) { int next = 0; for (auto i = peers.rbegin(); i != peers.rend(); ++i) { if (*i >= 0) next = *i; else *i = next; } };
        leading(left_peer_); leading(right_peer_);
        for (const auto& move : moves_) for (std::size_t i = 0; i < move.lines; ++i) { source_left_.Call(SCI_MARKERADD,source_left_.NativeLine(move.left + i),26); source_right_.Call(SCI_MARKERADD,source_right_.NativeLine(move.right + i),26); }
        SourceGapPlan gaps;
        if (BuildSourceGaps(text_rows_,{model_->base.size(),model_->left.size(),model_->right.size()},model_->three_way,&gaps)) {
            source_aligned_ = true;
            CCodeEditor* editors[] = {&source_base_,&source_left_,&source_right_};
            for (unsigned side = model_->three_way ? 0u : 1u; side < 3; ++side) for (const auto& gap : gaps[side]) { std::string text; text.reserve(gap.lines * 2); for (std::size_t i = 0; i < gap.lines; ++i) { if (i) text += '\n'; text += ' '; } editors[side]->Call(SCI_ANNOTATIONSETTEXT,gap.after_line,reinterpret_cast<LPARAM>(text.c_str())); editors[side]->Call(SCI_ANNOTATIONSETSTYLE,gap.after_line,250); }
        }
    }
    ShowAnchorMarkers(); source_sync_ = false;
}
void CAdvancedCompareView::SourceChanged(unsigned side) {
    CCodeEditor* editors[] = {&source_base_,&source_left_,&source_right_}; auto& from = *editors[side];
    if (source_sync_ || !source_aligned_ || stale_ || !model_ || ::GetFocus() != from.GetSafeHwnd()) return;
    source_sync_ = true;
    const auto top = from.Call(SCI_GETFIRSTVISIBLELINE); auto& previous = source_top_[side];
    if (top != previous) { previous = top; for (unsigned target = model_->three_way ? 0u : 1u; target < 3; ++target) if (target != side) editors[target]->Call(SCI_SETFIRSTVISIBLELINE,top); }
    if (SideBySide()) {
        review_focus_ = &from;
        const auto line = from.OriginalLine(from.Call(SCI_LINEFROMPOSITION,from.Call(SCI_GETCURRENTPOS)));
        for (std::size_t i = 0; i < visible_.size(); ++i) {
            const auto& row = text_rows_[visible_[i]];
            if ((side == 1 ? row.left : row.right) != line || context_rows_.hidden.count(visible_[i])) continue;
            if (!(rows_.GetItemState(static_cast<int>(i),LVIS_SELECTED) & LVIS_SELECTED)) {
                rows_.SetItemState(-1,0,LVIS_SELECTED); rows_.SetItemState(static_cast<int>(i),LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED);
                rows_.EnsureVisible(static_cast<int>(i),FALSE);
            }
            break;
        }
    }
    source_sync_ = false;
}
void CAdvancedCompareView::SelectSourceRow(std::size_t row) {
    if (!model_ || stale_ || row >= text_rows_.size()) return;
    const auto& item = text_rows_[row];
    source_sync_ = true;
    for (const auto& side : {std::make_pair(&source_left_,item.left),std::make_pair(&source_right_,item.right),std::make_pair(&source_base_,item.base)}) if (side.second >= 0 && side.second < side.first->Call(SCI_GETLINECOUNT)) {
        const auto line = side.first->NativeLine(side.second),at = side.first->Call(SCI_POSITIONFROMLINE,line); side.first->Call(SCI_SETSEL,at,side.first->Call(SCI_GETLINEENDPOSITION,line)); side.first->Call(SCI_SETFIRSTVISIBLELINE,static_cast<std::intptr_t>(row));
    }
    source_sync_ = false;
}
void CAdvancedCompareView::OnSourceRowChanged(NMHDR* header,LRESULT* result) {
    *result = 0; const auto* item = reinterpret_cast<NMLISTVIEW*>(header);
    if (!source_sync_ && (item->uNewState & LVIS_SELECTED) && item->iItem >= 0 && static_cast<std::size_t>(item->iItem) < visible_.size()) SelectSourceRow(visible_[item->iItem]);
}
void CAdvancedCompareView::ReviewMovedBlocks() {
    if (Busy() || stale_ || !model_ || model_->three_way) { Status(L"Run a two-way Text comparison before reviewing moved blocks."); return; }
    if (moves_.empty()) { Status(L"No unique exact moved blocks found (minimum two complete unmatched lines, including EOL). Ambiguous repeated blocks are skipped."); return; }
    const auto model = model_; std::vector<std::vector<std::wstring>> rows;
    for (const auto& move : moves_) rows.push_back({std::to_wstring(move.left + 1),std::to_wstring(move.right + 1),std::to_wstring(move.lines),model->left[move.left].text.substr(0,200)});
    CReviewList dialog(this,L"Moved blocks - exact pairs (up to 4096)",{L"Left line",L"Right line",L"Lines",L"Preview"},std::move(rows)); dialog.accept_label = L"Go to pair";
    modal_ = true; const auto answer = dialog.DoModal(); modal_ = false;
    if (answer != IDOK || dialog.selected.empty() || model_ != model || stale_) return;
    const auto move = moves_[dialog.selected[0]]; source_sync_ = true;
    CCodeEditor* left = pasted_inputs_ ? &input_left_ : &source_left_; CCodeEditor* right = pasted_inputs_ ? &input_right_ : &source_right_;
    for (const auto& side : {std::make_pair(left,move.left),std::make_pair(right,move.right)}) { const auto start = side.first->Call(SCI_POSITIONFROMLINE,side.first->NativeLine(side.second)),end = side.first->Call(SCI_POSITIONFROMLINE,side.first->NativeLine(side.second + move.lines)); side.first->Call(SCI_SETSEL,start,end < 0 ? side.first->Call(SCI_GETLENGTH) : end); side.first->Call(SCI_SCROLLCARET); }
    source_sync_ = false; Status(L"Exact moved-block pair selected. Merge choices and report contents are unchanged.");
}
void CAdvancedCompareView::ToggleBaseSource() { if (!Busy() && mode_ == 3) { show_base_source_ = !show_base_source_; Layout(); } }

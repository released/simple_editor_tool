#include "advanced_compare.h"
#include <algorithm>
#include <set>
using namespace simple_editor::core;
void CAdvancedCompareView::OnDraw(NMHDR* header,LRESULT* result) {
    auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(header); *result = CDRF_DODEFAULT;
    if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) { *result = CDRF_NOTIFYITEMDRAW; return; }
    if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        const auto row = static_cast<std::size_t>(draw->nmcd.dwItemSpec); if (row >= visible_.size()) return;
        if (mode_ == 4 && scan_) { const auto index = visible_[row]; auto state = scan_->rows[index].state; if (folder_tree_ && tree_rows_.errors[index]) state = FolderState::Error; else if (folder_tree_ && tree_rows_.differences[index] && state == FolderState::Equal) state = FolderState::Different; draw->clrTextBk = state == FolderState::Equal ? RGB(255,255,255) : state == FolderState::Error || state == FolderState::TypeConflict ? RGB(255,215,215) : RGB(255,248,205); }
        else if (model_) { const auto& block = model_->blocks[text_rows_[visible_[row]].block]; draw->clrTextBk = !block.changed ? RGB(255,255,255) : block.conflict && block.choice == MergeChoice::Unresolved && !manual_resolved_ ? RGB(255,210,210) : !block.important ? RGB(225,240,255) : RGB(255,248,205); }
        if (theme_ == 1) { const bool equal = draw->clrTextBk == RGB(255,255,255),conflict = draw->clrTextBk == RGB(255,210,210) || draw->clrTextBk == RGB(255,215,215); draw->clrTextBk = equal ? RGB(31,34,42) : conflict ? RGB(90,40,45) : RGB(70,60,25); draw->clrText = RGB(225,228,235); }
        else if (theme_ == 2) { draw->clrTextBk = GetSysColor(COLOR_WINDOW); draw->clrText = GetSysColor(COLOR_WINDOWTEXT); }
        *result = CDRF_NOTIFYSUBITEMDRAW; return;
    }
    if (draw->nmcd.dwDrawStage != (CDDS_ITEMPREPAINT | CDDS_SUBITEM) || mode_ == 4 || !model_ || draw->iSubItem >= 6 || !(draw->iSubItem % 2)) return;
    const auto row = static_cast<std::size_t>(draw->nmcd.dwItemSpec); if (row >= visible_.size()) return;
    if (context_rows_.hidden.count(visible_[row])) return;
    CRect bounds; rows_.GetSubItemRect(static_cast<int>(row),draw->iSubItem,LVIR_LABEL,bounds); if (bounds.Width() <= 0) return;
    const auto text = Cell(static_cast<int>(row),draw->iSubItem); const bool selected = (draw->nmcd.uItemState & CDIS_SELECTED) != 0;
    const auto foreground = selected ? GetSysColor(COLOR_HIGHLIGHTTEXT) : theme_ == 2 ? GetSysColor(COLOR_WINDOWTEXT) : theme_ == 1 ? RGB(225,228,235) : RGB(35,35,35), background = selected ? GetSysColor(COLOR_HIGHLIGHT) : draw->clrTextBk;
    std::vector<COLORREF> colors(text.size(),foreground); std::vector<bool> highlighted(text.size(),false);
    if (syntax_.GetCheck() && !selected && theme_ != 2) {
        static const std::set<std::wstring> keywords{L"if",L"else",L"for",L"while",L"return",L"class",L"struct",L"int",L"void",L"bool",L"const",L"auto",L"static",L"public",L"private",L"namespace",L"def",L"import",L"from",L"try",L"catch",L"except",L"function",L"let",L"var",L"true",L"false",L"null",L"nullptr"};
        for (std::size_t i = 0; i < text.size();) { const auto begin = i; auto color = foreground;
            if (text[i] == L'#' || (text[i] == L'/' && i + 1 < text.size() && text[i + 1] == L'/')) { i = text.size(); color = RGB(40,125,55); }
            else if (text[i] == L'\'' || text[i] == L'"') { const auto quote = text[i++]; while (i < text.size()) { if (text[i] == L'\\' && i + 1 < text.size()) i += 2; else if (text[i++] == quote) break; } color = RGB(160,65,35); }
            else if (iswalnum(text[i]) || text[i] == L'_') { while (i < text.size() && (iswalnum(text[i]) || text[i] == L'_')) ++i; if (keywords.count(text.substr(begin,i - begin))) color = RGB(25,65,190); else if (iswdigit(text[begin])) color = RGB(120,45,155); }
            else ++i;
            if (theme_ == 1 && color != foreground) color = color == RGB(40,125,55) ? RGB(120,190,120) : color == RGB(160,65,35) ? RGB(230,160,110) : color == RGB(25,65,190) ? RGB(120,170,255) : RGB(190,130,230);
            std::fill(colors.begin() + begin,colors.begin() + i,color);
        }
    }
    if (presentation_ && visible_[row] < presentation_->characters.size() && (draw->iSubItem == 3 || draw->iSubItem == 5)) {
        const auto& differences = presentation_->characters[visible_[row]]; const auto& spans = draw->iSubItem == 3 ? differences.left : differences.right;
        for (const auto& span : spans) for (auto i = span.begin; i < (std::min)(text.size(),span.begin + span.length); ++i) highlighted[i] = true;
    }
    CDC* dc = CDC::FromHandle(draw->nmcd.hdc); const int saved = dc->SaveDC(); dc->IntersectClipRect(bounds); dc->FillSolidRect(bounds,background); dc->SetBkMode(TRANSPARENT); dc->SelectObject(&content_font_); int x = bounds.left + 4;
    const auto& source = text_rows_[visible_[row]]; const bool insertion = draw->iSubItem == 5 && source.left < 0;
    for (std::size_t i = 0; i < text.size() && x < bounds.right;) { auto end = i + 1; while (end < text.size() && colors[end] == colors[i] && highlighted[end] == highlighted[i]) ++end;
        std::wstring segment; for (auto n = i; n < end; ++n) { if (text[n] == L'\t') segment += L"    "; else segment.push_back(text[n]); }
        const auto width = dc->GetTextExtent(segment.c_str(),static_cast<int>(segment.size())).cx;
        if (highlighted[i]) dc->FillSolidRect(x,bounds.top,width,bounds.Height(),theme_ == 2 ? GetSysColor(COLOR_HIGHLIGHT) : selected ? RGB(150,65,75) : theme_ == 1 ? insertion ? RGB(35,65,45) : RGB(110,45,60) : insertion ? RGB(165,235,180) : RGB(255,165,170));
        if (theme_ == 2 && highlighted[i]) colors[i] = GetSysColor(COLOR_HIGHLIGHTTEXT);
        dc->SetTextColor(colors[i]); dc->TextOutW(x,bounds.top,segment.c_str(),static_cast<int>(segment.size())); x += width; i = end;
    }
    dc->RestoreDC(saved); *result = CDRF_SKIPDEFAULT;
}

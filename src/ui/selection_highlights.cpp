#include "code_editor.h"
#include <algorithm>
#include <cwctype>

namespace {
constexpr UINT_PTR occurrence_timer = 0x5348;
constexpr unsigned occurrence_indicator = 14;
struct IndicatorScope {
    CCodeEditor& editor;
    LRESULT current,value;
    explicit IndicatorScope(CCodeEditor& owner) : editor(owner),current(owner.Call(SCI_GETINDICATORCURRENT)),value(owner.Call(SCI_GETINDICATORVALUE)) {
        editor.Call(SCI_SETINDICATORCURRENT,occurrence_indicator); editor.Call(SCI_SETINDICATORVALUE,1);
    }
    ~IndicatorScope() { editor.Call(SCI_SETINDICATORCURRENT,current); editor.Call(SCI_SETINDICATORVALUE,value); }
};
}

void CCodeEditor::QueueSelectionHighlights(bool force) {
    if (!GetSafeHwnd() || ::GetFocus() != GetSafeHwnd() || !IsWindowVisible()) return;
    occurrence_force_ = occurrence_force_ || force;
    if (!occurrence_queued_ || force) occurrence_queued_ = SetTimer(occurrence_timer,80,nullptr) != 0;
}

void CCodeEditor::ClearSelectionHighlights() {
    if (!GetSafeHwnd()) return;
    KillTimer(occurrence_timer); occurrence_queued_ = false; occurrence_force_ = false;
    { IndicatorScope indicator(*this); Call(SCI_INDICATORCLEARRANGE,0,Call(SCI_GETLENGTH)); }
    occurrence_text_.clear(); occurrence_ranges_.clear(); occurrence_complete_ = true; occurrence_document_ = 0;
}

void CCodeEditor::OnTimer(UINT_PTR timer) {
    if (timer != occurrence_timer) { CEdit::OnTimer(timer); return; }
    KillTimer(timer); occurrence_queued_ = false;
    if (::GetFocus() != GetSafeHwnd() || !IsWindowVisible()) return;
    try { UpdateSelectionHighlights(); }
    catch (...) { ClearSelectionHighlights(); }
}

void CCodeEditor::UpdateSelectionHighlights() {
    const auto start = Call(SCI_GETSELECTIONSTART),end = Call(SCI_GETSELECTIONEND),length = Call(SCI_GETLENGTH);
    const auto document = Call(SCI_GETDOCPOINTER);
    std::string needle;
    if (Call(SCI_GETSELECTIONS) == 1 && Call(SCI_GETSELECTIONMODE) == SC_SEL_STREAM &&
        start >= ContentStart() && end > start && end - start <= 1024) {
        needle.resize(static_cast<std::size_t>(end - start) + 1);
        Sci_TextRangeFull selected{{start,end},needle.data()}; Call(SCI_GETTEXTRANGEFULL,0,reinterpret_cast<LPARAM>(&selected)); needle.pop_back();
        const auto wide = FromUtf8(needle);
        if (needle.find_first_of("\r\n\0",0,3) != std::string::npos || wide.empty() ||
            std::all_of(wide.begin(),wide.end(),[](wchar_t ch) { return iswspace(ch) != 0; })) needle.clear();
    }
    if (needle.empty()) {
        // Indicators live in the shared document; the focused clone also clears its previous owner's marks.
        ClearSelectionHighlights(); return;
    }

    std::vector<std::pair<std::intptr_t,std::intptr_t>> ranges;
    CRect bounds; GetClientRect(bounds);
    const auto row_height = (std::max<LRESULT>)(1,Call(SCI_TEXTHEIGHT,0));
    const auto rows = (std::min<LRESULT>)(4096,bounds.Height() / row_height + 1);
    ranges.reserve(static_cast<std::size_t>(rows));
    for (LRESULT row = 0; row < rows; ++row) {
        const auto y = row * row_height + 1;
        const auto left = Call(SCI_POSITIONFROMPOINT,0,y),right = Call(SCI_POSITIONFROMPOINT,(std::max)(0,bounds.Width() - 1),y);
        if (left < 0 || right < left) continue;
        // Expand around each visible display row so horizontally clipped/wrapped matches are complete.
        const auto first = (std::max)(ContentStart(),left - static_cast<LRESULT>(needle.size()) + 1);
        const auto last = (std::min)(length,right + static_cast<LRESULT>(needle.size()));
        if (last <= first) continue;
        if (!ranges.empty() && first <= ranges.back().second) ranges.back().second = (std::max)(ranges.back().second,last);
        else ranges.emplace_back(first,last);
    }
    if (occurrence_force_ || document != occurrence_document_ || generation_ != occurrence_generation_ ||
        needle != occurrence_text_ || ranges != occurrence_ranges_) {
        IndicatorScope indicator(*this); Call(SCI_INDICATORCLEARRANGE,0,length);
        occurrence_text_ = std::move(needle); occurrence_ranges_ = std::move(ranges);
        occurrence_document_ = document; occurrence_generation_ = generation_; occurrence_range_ = 0;
        occurrence_at_ = occurrence_ranges_.empty() ? 0 : occurrence_ranges_.front().first;
        occurrence_force_ = false; occurrence_complete_ = occurrence_ranges_.empty();
    }
    if (occurrence_complete_) return;
    IndicatorScope indicator(*this);
    const auto deadline = GetTickCount64() + 4; unsigned matches = 0; std::intptr_t scanned = 0;
    while (occurrence_range_ < occurrence_ranges_.size() && matches < 512 && scanned < 65536 && GetTickCount64() < deadline) {
        const auto stop = occurrence_ranges_[occurrence_range_].second;
        const auto chunk = (std::min)(stop,occurrence_at_ + 32768);
        const auto search_end = (std::min)(stop,chunk + static_cast<LRESULT>(occurrence_text_.size()) - 1);
        Sci_TextToFindFull find{{occurrence_at_,search_end},occurrence_text_.c_str(),{0,0}};
        const auto found = Call(SCI_FINDTEXTFULL,SCFIND_MATCHCASE,reinterpret_cast<LPARAM>(&find));
        if (found >= occurrence_at_ && found < chunk && find.chrgText.cpMax <= stop) {
            Call(SCI_INDICATORFILLRANGE,found,find.chrgText.cpMax - found);
            const auto next = Call(SCI_POSITIONAFTER,found); scanned += next - occurrence_at_; occurrence_at_ = next; ++matches;
        } else { scanned += chunk - occurrence_at_; occurrence_at_ = chunk; }
        if (occurrence_at_ >= stop) {
            ++occurrence_range_; if (occurrence_range_ < occurrence_ranges_.size()) occurrence_at_ = occurrence_ranges_[occurrence_range_].first;
        }
    }
    occurrence_complete_ = occurrence_range_ == occurrence_ranges_.size();
    if (!occurrence_complete_) occurrence_queued_ = SetTimer(occurrence_timer,15,nullptr) != 0;
}

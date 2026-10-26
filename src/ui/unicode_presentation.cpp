#include "code_editor.h"
using namespace simple_editor::core;
void CCodeEditor::ClearUnicodeNotices() {
    if (!GetSafeHwnd()) return;
    Call(SCI_SETINDICATORCURRENT,13); Call(SCI_INDICATORCLEARRANGE,0,Call(SCI_GETLENGTH));
    for (const auto point : UnicodeNoticePoints()) { const auto text = ToUtf8(std::wstring(1,static_cast<wchar_t>(point))); Call(SCI_CLEARREPRESENTATION,reinterpret_cast<WPARAM>(text.c_str())); }
}
void CCodeEditor::ShowUnicodeNotices(const UnicodeNotices& notices,const std::set<std::uint32_t>& allowed) {
    // A plain underline also handles zero-width glyph rectangles in the GDI renderer.
    ClearUnicodeNotices(); Call(SCI_INDICSETSTYLE,13,INDIC_PLAIN); Call(SCI_INDICSETFORE,13,RGB(255,145,45)); Call(SCI_INDICSETSTROKEWIDTH,13,200); Call(SCI_SETINDICATORCURRENT,13);
    for (const auto point : UnicodeNoticePoints()) if (!allowed.count(point)) { const auto text = ToUtf8(std::wstring(1,static_cast<wchar_t>(point))); char label[20]{}; sprintf_s(label,"U+%04X",point); Call(SCI_SETREPRESENTATION,reinterpret_cast<WPARAM>(text.c_str()),reinterpret_cast<LPARAM>(label)); }
    for (const auto& notice : notices.rows) Call(SCI_INDICATORFILLRANGE,NativePosition(notice.offset),notice.length);
}

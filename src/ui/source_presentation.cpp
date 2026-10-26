#include "code_editor.h"
#include <algorithm>
#include <vector>
std::intptr_t CCodeEditor::ContentStart() const { return source_presentation_ ? (std::max<std::intptr_t>)(0,Call(SCI_POSITIONFROMLINE,1)) : 0; }
std::intptr_t CCodeEditor::NativePosition(std::intptr_t content) const { return (std::clamp)(content,static_cast<std::intptr_t>(0),Call(SCI_GETLENGTH) - ContentStart()) + ContentStart(); }
std::intptr_t CCodeEditor::ContentPosition(std::intptr_t native) const { return (std::clamp)(native,ContentStart(),Call(SCI_GETLENGTH)) - ContentStart(); }
std::intptr_t CCodeEditor::OriginalLine(std::intptr_t native) const { return (std::max<std::intptr_t>)(0,native - (source_presentation_ ? 1 : 0)); }
void CCodeEditor::SetSourcePresentation(bool enabled) {
    if (enabled == source_presentation_) return; const auto text = Utf8(); source_presentation_ = enabled;
    if (!GetSafeHwnd()) return; Call(SCI_SETMARGINTYPEN,0,enabled ? SC_MARGIN_TEXT : SC_MARGIN_NUMBER); SetWindowTextW(FromUtf8(text).c_str()); SetWrap(false);
    Call(SCI_ANNOTATIONSETVISIBLE,ANNOTATION_STANDARD); Call(SCI_SETSELECTIONMODE,SC_SEL_STREAM);
}
bool CCodeEditor::SourceMessage(UINT message,WPARAM wparam,LPARAM, LRESULT* result) {
    if (message == SCI_SETSELECTIONMODE && wparam != SC_SEL_STREAM) { *result = 0; return true; }
    if (message == SCI_FOLDALL || message == SCI_FOLDLINE || message == SCI_FOLDCHILDREN || message == SCI_TOGGLEFOLD || message == SCI_HIDELINES) { *result = 0; return true; }
    const bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const auto command = message == WM_COMMAND ? LOWORD(wparam) : message;
    if (command == SCI_SELECTALL || (message == WM_KEYDOWN && control && wparam == 'A')) { Call(SCI_SETSEL,ContentStart(),Call(SCI_GETLENGTH)); *result = 0; return true; }
    if (command == SCI_DOCUMENTSTART || command == SCI_DOCUMENTSTARTEXTEND || (message == WM_KEYDOWN && control && wparam == VK_HOME)) {
        const bool extend = command == SCI_DOCUMENTSTARTEXTEND || (GetKeyState(VK_SHIFT) & 0x8000); if (extend) Call(SCI_SETSEL,Call(SCI_GETANCHOR),ContentStart()); else Call(SCI_SETEMPTYSELECTION,ContentStart()); *result = 0; return true;
    }
    if (message == WM_CUT || command == SCI_CUT) { *result = 0; return true; }
    const bool copy = message == WM_COPY || command == SCI_COPY || command == SCI_COPYALLOWLINE || (message == WM_KEYDOWN && ((control && wparam == 'C') || (control && wparam == VK_INSERT)));
    if (!copy) return false;
    const auto count = Call(SCI_GETSELECTIONS),main = Call(SCI_GETMAINSELECTION); std::vector<std::pair<std::intptr_t,std::intptr_t>> selections;
    for (std::intptr_t i = 0; i < count; ++i) { const auto anchor = Call(SCI_GETSELECTIONNANCHOR,i),caret = Call(SCI_GETSELECTIONNCARET,i); selections.emplace_back(anchor,caret); Call(SCI_SETSELECTIONNANCHOR,i,(std::max)(ContentStart(),anchor)); Call(SCI_SETSELECTIONNCARET,i,(std::max)(ContentStart(),caret)); }
    *result = CEdit::WindowProc(SCI_COPY,0,0);
    for (std::intptr_t i = 0; i < count; ++i) { Call(SCI_SETSELECTIONNANCHOR,i,selections[i].first); Call(SCI_SETSELECTIONNCARET,i,selections[i].second); } Call(SCI_SETMAINSELECTION,main); return true;
}

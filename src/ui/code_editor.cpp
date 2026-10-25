#include "code_editor.h"
#include <algorithm>
#include <filesystem>
#include <vector>
#include <climits>
#include <cstring>
#include <Shellapi.h>
#include "../../vendor/scintilla/include/ILexer.h"
#include "../../vendor/lexilla/include/SciLexer.h"

extern "C" int Scintilla_RegisterClasses(void* instance);
extern "C" int Scintilla_ReleaseResources();
extern "C" Scintilla::ILexer5* EditorCreateLexer(const char* name);
void CCodeEditor::ReleaseResources() { Scintilla_ReleaseResources(); }

BEGIN_MESSAGE_MAP(CCodeEditor,CEdit)
    ON_NOTIFY_REFLECT(SCN_MODIFIED,OnNotification)
    ON_NOTIFY_REFLECT(SCN_UPDATEUI,OnNotification)
    ON_NOTIFY_REFLECT(SCN_CHARADDED,OnNotification)
    ON_NOTIFY_REFLECT(SCN_MARGINCLICK,OnNotification)
END_MESSAGE_MAP()
std::string CCodeEditor::ToUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);
    if (!count) return {}; std::string result(count,'\0'); WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),result.data(),count,nullptr,nullptr); return result;
}
std::wstring CCodeEditor::FromUtf8(const std::string& value) {
    if (value.empty()) return {}; const int count = MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0);
    if (!count) return {}; std::wstring result(count,L'\0'); MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),result.data(),count); return result;
}
bool CCodeEditor::InitializeResources() { static const bool registered = Scintilla_RegisterClasses(GetModuleHandleW(nullptr)) != 0; return registered; }
BOOL CCodeEditor::Create(DWORD style, const RECT& bounds, CWnd* parent, UINT id) {
    if (!InitializeResources() || !CWnd::CreateEx(0,L"Scintilla",nullptr,style,bounds,parent,id)) return FALSE;
    Call(SCI_SETCODEPAGE,SC_CP_UTF8); Call(SCI_SETMODEVENTMASK,SC_MODEVENTMASKALL); Call(SCI_SETCOMMANDEVENTS,0);
    Call(SCI_SETMULTIPLESELECTION,1); Call(SCI_SETADDITIONALSELECTIONTYPING,1); Call(SCI_SETMULTIPASTE,SC_MULTIPASTE_EACH);
    Call(SCI_SETRECTANGULARSELECTIONMODIFIER,SCMOD_ALT); Call(SCI_SETSCROLLWIDTHTRACKING,1);
    Call(SCI_SETMARGINTYPEN,0,SC_MARGIN_NUMBER); Call(SCI_SETMARGINTYPEN,1,SC_MARGIN_SYMBOL); Call(SCI_SETMARGINMASKN,1,1 << 20); Call(SCI_SETMARGINSENSITIVEN,1,1);
    Call(SCI_MARKERDEFINE,20,SC_MARK_CIRCLE); Call(SCI_MARKERSETFORE,20,RGB(25,85,200)); Call(SCI_MARKERSETBACK,20,RGB(80,140,240)); Call(SCI_SETMARGINWIDTHN,1,16);
    Call(SCI_SETMARGINTYPEN,3,SC_MARGIN_SYMBOL); Call(SCI_SETMARGINMASKN,3,(1 << 21) | (1 << 22) | (1 << 23) | (1 << 24)); Call(SCI_SETMARGINWIDTHN,3,6); Call(SCI_SETMARGINTYPEN,2,SC_MARGIN_SYMBOL); Call(SCI_SETMARGINMASKN,2,SC_MASK_FOLDERS); Call(SCI_SETMARGINSENSITIVEN,2,1); Call(SCI_SETMARGINWIDTHN,2,16);
    for (int marker = SC_MARKNUM_FOLDEREND; marker <= SC_MARKNUM_FOLDEROPEN; ++marker) { Call(SCI_MARKERDEFINE,marker,marker == SC_MARKNUM_FOLDER || marker == SC_MARKNUM_FOLDEREND ? SC_MARK_BOXPLUS : SC_MARK_BOXMINUS); Call(SCI_MARKERSETFORE,marker,RGB(255,255,255)); Call(SCI_MARKERSETBACK,marker,RGB(100,110,125)); }
    Call(SCI_SETAUTOMATICFOLD,SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CHANGE); SetTabWidth(4,false);
    Call(SCI_SETPROPERTY,reinterpret_cast<WPARAM>("fold"),reinterpret_cast<LPARAM>("1")); ConfigureLanguage(L""); SetDark(false); UpdateMargins();
    if (style & ES_READONLY) Call(SCI_SETREADONLY,1); InitializeDropTarget(); return TRUE;
}
std::string CCodeEditor::Utf8() const { const auto count = static_cast<std::size_t>(Call(SCI_GETLENGTH)); std::string text(count + 1,'\0'); Call(SCI_GETTEXT,text.size(),reinterpret_cast<LPARAM>(text.data())); text.resize(count); return text; }
std::intptr_t CCodeEditor::BytePosition(std::intptr_t units) const { const auto length = Call(SCI_GETLENGTH); return units < 0 ? length : Call(SCI_POSITIONRELATIVECODEUNITS,0,(std::min)(units,UnitPosition(length))); }
std::intptr_t CCodeEditor::UnitPosition(std::intptr_t bytes) const { return Call(SCI_COUNTCODEUNITS,0,(std::max<std::intptr_t>)(0,bytes)); }
void CCodeEditor::UpdateMargins() { if (!GetSafeHwnd()) return; const auto label = " " + std::to_string(Call(SCI_GETLINECOUNT)) + " "; Call(SCI_SETMARGINWIDTHN,0,Call(SCI_TEXTWIDTH,STYLE_LINENUMBER,reinterpret_cast<LPARAM>(label.c_str())) + 4); }
void CCodeEditor::ApplyFont() {
    LOGFONT font{}; if (!borrowed_font_ || !GetObjectW(borrowed_font_,sizeof(font),&font)) return;
    const auto face = ToUtf8(font.lfFaceName); CClientDC dc(this); const int size = MulDiv(abs(font.lfHeight),7200,dc.GetDeviceCaps(LOGPIXELSY));
    Call(SCI_STYLESETFONT,STYLE_DEFAULT,reinterpret_cast<LPARAM>(face.c_str())); Call(SCI_STYLESETSIZEFRACTIONAL,STYLE_DEFAULT,(std::max)(100,size));
    Call(SCI_STYLECLEARALL); SetTheme(theme_); UpdateMargins();
}
void CCodeEditor::SetDark(bool dark) { SetTheme(dark ? 1 : 0); }
void CCodeEditor::SetTheme(unsigned theme) {
    theme_ = (std::min)(2u,theme); if (!GetSafeHwnd()) return; const bool dark = theme_ == 1,contrast = theme_ == 2; dark_ = dark; const auto fore = contrast ? GetSysColor(COLOR_WINDOWTEXT) : dark ? RGB(225,228,235) : RGB(30,35,42), back = contrast ? GetSysColor(COLOR_WINDOW) : dark ? RGB(31,34,42) : RGB(255,255,255);
    for (unsigned style = 0; style < 256; ++style) { Call(SCI_STYLESETFORE,style,fore); Call(SCI_STYLESETBACK,style,back); }
    const auto green = dark ? RGB(120,190,120) : RGB(35,125,55), blue = dark ? RGB(120,170,255) : RGB(25,65,195), red = dark ? RGB(230,160,110) : RGB(160,65,35);
    if (!lexer_.empty() && !contrast) {
        for (int style : {1,2,3}) Call(SCI_STYLESETFORE,style,green);
        Call(SCI_STYLESETFORE,5,blue); Call(SCI_STYLESETBOLD,5,1); for (int style : {6,7,12,13}) Call(SCI_STYLESETFORE,style,red); Call(SCI_STYLESETFORE,4,RGB(155,85,180)); Call(SCI_STYLESETFORE,9,blue);
        if (lexer_ == "python") { Call(SCI_STYLESETFORE,SCE_P_NUMBER,RGB(155,85,180)); for (int style : {SCE_P_STRING,SCE_P_CHARACTER,SCE_P_TRIPLE,SCE_P_TRIPLEDOUBLE}) Call(SCI_STYLESETFORE,style,red); Call(SCI_STYLESETFORE,SCE_P_COMMENTLINE,green); }
        if (lexer_ == "json") { Call(SCI_STYLESETFORE,SCE_JSON_STRING,red); Call(SCI_STYLESETFORE,SCE_JSON_NUMBER,RGB(155,85,180)); Call(SCI_STYLESETFORE,SCE_JSON_KEYWORD,blue); }
    }
    Call(SCI_STYLESETFORE,STYLE_LINENUMBER,contrast ? fore : dark ? RGB(160,165,180) : RGB(100,105,115)); Call(SCI_STYLESETBACK,STYLE_LINENUMBER,contrast ? back : dark ? RGB(40,44,53) : RGB(242,244,248));
    Call(SCI_SETCARETFORE,fore); Call(SCI_SETSELFORE,1,contrast ? GetSysColor(COLOR_HIGHLIGHTTEXT) : RGB(255,255,255)); Call(SCI_SETSELBACK,1,contrast ? GetSysColor(COLOR_HIGHLIGHT) : RGB(60,105,170)); Call(SCI_SETFOLDMARGINCOLOUR,1,back); Call(SCI_SETFOLDMARGINHICOLOUR,1,back); Invalidate();
}
void CCodeEditor::ConfigureLanguage(const std::wstring& path) {
    if (!GetSafeHwnd()) return; auto ext = std::filesystem::path(path).extension().wstring(); for (auto& c : ext) c = static_cast<wchar_t>(towlower(c)); lexer_.clear();
    if (ext == L".c" || ext == L".cpp" || ext == L".h" || ext == L".hpp" || ext == L".cxx" || ext == L".cs" || ext == L".js" || ext == L".ts" || ext == L".java") lexer_ = "cpp";
    else if (ext == L".py") lexer_ = "python"; else if (ext == L".json") lexer_ = "json"; else if (ext == L".html" || ext == L".xml" || ext == L".htm") lexer_ = "hypertext";
    else if (ext == L".ps1") lexer_ = "powershell"; else if (ext == L".md") lexer_ = "markdown"; else if (ext == L".sql") lexer_ = "sql";
    else if (ext == L".yaml" || ext == L".yml") lexer_ = "yaml"; else if (ext == L".sh") lexer_ = "bash"; else if (ext == L".bat" || ext == L".cmd") lexer_ = "batch"; else if (ext == L".diff" || ext == L".patch") lexer_ = "diff"; else if (ext == L".cmake" || std::filesystem::path(path).filename() == L"CMakeLists.txt") lexer_ = "cmake";
    Call(SCI_SETILEXER,0,reinterpret_cast<LPARAM>(EditorCreateLexer(lexer_.c_str())));
    Call(SCI_SETPROPERTY,reinterpret_cast<WPARAM>("fold"),reinterpret_cast<LPARAM>("1"));
    words_ = lexer_ == "python" ? "and as assert async await break class continue def del elif else except False finally for from global if import in is lambda None nonlocal not or pass raise return True try while with yield" : lexer_ == "json" ? "true false null" : "auto bool break case catch char class const constexpr continue default delete do double else enum false float for if import int long namespace new nullptr private protected public return short signed sizeof static struct switch template this throw true try typedef typename union unsigned using virtual void volatile while";
    Call(SCI_SETKEYWORDS,0,reinterpret_cast<LPARAM>(words_.c_str())); Call(SCI_COLOURISE,0,-1); SetTheme(theme_);
}
void CCodeEditor::SetWrap(bool value) { Call(SCI_SETWRAPMODE,value ? SC_WRAP_WORD : SC_WRAP_NONE); }
void CCodeEditor::SetWhitespace(bool value) { Call(SCI_SETVIEWWS,value ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE); Call(SCI_SETVIEWEOL,value); }
void CCodeEditor::SetTabWidth(unsigned width, bool use_tabs) { Call(SCI_SETTABWIDTH,(std::clamp)(width,1u,16u)); Call(SCI_SETINDENT,(std::clamp)(width,1u,16u)); Call(SCI_SETUSETABS,use_tabs); Call(SCI_SETINDENTATIONGUIDES,SC_IV_LOOKBOTH); }
void CCodeEditor::ToggleBookmark() { const auto line = Call(SCI_LINEFROMPOSITION,Call(SCI_GETCURRENTPOS)); if (Call(SCI_MARKERGET,line) & (1 << 20)) Call(SCI_MARKERDELETE,line,20); else Call(SCI_MARKERADD,line,20); }
void CCodeEditor::NextBookmark(bool previous) { const auto line = Call(SCI_LINEFROMPOSITION,Call(SCI_GETCURRENTPOS)); auto found = Call(previous ? SCI_MARKERPREVIOUS : SCI_MARKERNEXT,previous ? line - 1 : line + 1,1 << 20); if (found < 0) found = Call(previous ? SCI_MARKERPREVIOUS : SCI_MARKERNEXT,previous ? Call(SCI_GETLINECOUNT) - 1 : 0,1 << 20); if (found >= 0) GoToLine(found + 1); }
void CCodeEditor::GoToLine(std::uint64_t line) { const auto count = static_cast<std::uint64_t>(Call(SCI_GETLINECOUNT)); if (!count) return; const auto target = static_cast<WPARAM>((std::clamp<std::uint64_t>)(line,1,count) - 1); Call(SCI_ENSUREVISIBLE,target); Call(SCI_GOTOLINE,target); SetFocus(); }
std::wstring CCodeEditor::Status() const { const auto position = Call(SCI_GETCURRENTPOS); return L"Ln " + std::to_wstring(Call(SCI_LINEFROMPOSITION,position) + 1) + L", Col " + std::to_wstring(Call(SCI_GETCOLUMN,position) + 1) + L"  Selected " + std::to_wstring(Call(SCI_GETSELECTIONEND) - Call(SCI_GETSELECTIONSTART)) + L" UTF-8 bytes"; }
void CCodeEditor::ReplaceGenerated(const std::wstring& text, int token) {
    const auto value = ToUtf8(text), old = Utf8(); std::size_t prefix = 0, suffix = 0;
    while (prefix < old.size() && prefix < value.size() && old[prefix] == value[prefix]) ++prefix;
    while (prefix && prefix < value.size() && (static_cast<unsigned char>(value[prefix]) & 0xC0) == 0x80) --prefix;
    while (suffix < old.size() - prefix && suffix < value.size() - prefix && old[old.size() - suffix - 1] == value[value.size() - suffix - 1]) ++suffix;
    while (suffix && (static_cast<unsigned char>(value[value.size() - suffix]) & 0xC0) == 0x80) --suffix;
    const bool previous = suppress_; suppress_ = true;
    Call(SCI_BEGINUNDOACTION); Call(SCI_ADDUNDOACTION,token,0);
    if (old != value) { Call(SCI_SETTARGETRANGE,prefix,old.size() - suffix); Call(SCI_REPLACETARGET,value.size() - prefix - suffix,reinterpret_cast<LPARAM>(value.data() + prefix)); }
    Call(SCI_ENDUNDOACTION); suppress_ = previous;
}
void CCodeEditor::OnNotification(NMHDR* header, LRESULT* result) {
    *result = 0; const auto* event = reinterpret_cast<const SCNotification*>(header);
    if (header->code == SCN_MODIFIED) {
        if ((event->modificationType & SC_MOD_CONTAINER) && undo_container) undo_container(event->token,(event->modificationType & SC_PERFORMED_UNDO) != 0);
        if (event->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) { ++generation_; UpdateMargins(); if (!suppress_) { forced_modified_ = false; GetParent()->SendMessage(WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(),EN_CHANGE),reinterpret_cast<LPARAM>(GetSafeHwnd())); } }
    } else if (header->code == SCN_UPDATEUI) {
        const auto position = Call(SCI_GETCURRENTPOS); const auto ch = position > 0 ? Call(SCI_GETCHARAT,position - 1) : 0;
        if (ch && strchr("()[]{}",static_cast<char>(ch))) { const auto other = Call(SCI_BRACEMATCH,position - 1,0); if (other >= 0) Call(SCI_BRACEHIGHLIGHT,position - 1,other); else Call(SCI_BRACEBADLIGHT,position - 1); } else Call(SCI_BRACEHIGHLIGHT,static_cast<WPARAM>(-1),-1);
        if (selection_changed) selection_changed();
    } else if (header->code == SCN_MARGINCLICK) { const auto line = Call(SCI_LINEFROMPOSITION,event->position); if (event->margin == 1) { if (Call(SCI_MARKERGET,line) & (1 << 20)) Call(SCI_MARKERDELETE,line,20); else Call(SCI_MARKERADD,line,20); } else if (event->margin == 2) Call(SCI_TOGGLEFOLD,line); }
    else if (header->code == SCN_CHARADDED && !suppress_ && event->ch == '\n') {
        const auto line = Call(SCI_LINEFROMPOSITION,Call(SCI_GETCURRENTPOS)); if (line > 0) { auto indent = Call(SCI_GETLINEINDENTATION,line - 1); const auto end = Call(SCI_GETLINEENDPOSITION,line - 1); if (end && Call(SCI_GETCHARAT,end - 1) == '{') indent += Call(SCI_GETTABWIDTH); Call(SCI_SETLINEINDENTATION,line,indent); Call(SCI_GOTOPOS,Call(SCI_GETLINEINDENTPOSITION,line)); }
    }
}
LRESULT CCodeEditor::WindowProc(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_NCDESTROY: DetachDropTarget(); return CEdit::WindowProc(message,wparam,lparam);
    case WM_DROPFILES: { auto* frame = GetParentFrame(); if (frame && frame->GetSafeHwnd()) frame->SendMessage(WM_DROPFILES,wparam,lparam); else DragFinish(reinterpret_cast<HDROP>(wparam)); return 0; }
    case WM_SETTEXT: { const std::wstring wide = lparam ? reinterpret_cast<const wchar_t*>(lparam) : L""; const auto value = ToUtf8(wide); if (!wide.empty() && value.empty()) return FALSE; const bool previous = suppress_; suppress_ = true; const auto readonly = Call(SCI_GETREADONLY); Call(SCI_SETREADONLY,0); Call(SCI_SETCHANGEHISTORY,SC_CHANGE_HISTORY_DISABLED); Call(SCI_SETTEXT,0,reinterpret_cast<LPARAM>(value.c_str())); Call(SCI_SETREADONLY,readonly); Call(SCI_EMPTYUNDOBUFFER); Call(SCI_SETSAVEPOINT); Call(SCI_SETCHANGEHISTORY,SC_CHANGE_HISTORY_ENABLED | SC_CHANGE_HISTORY_MARKERS); forced_modified_ = false; suppress_ = previous; UpdateMargins(); return TRUE; }
    case WM_GETTEXTLENGTH: return UnitPosition(Call(SCI_GETLENGTH));
    case WM_GETTEXT: { if (!wparam || !lparam) return 0; const auto text = FromUtf8(Utf8()); const auto count = (std::min<std::size_t>)(text.size(),wparam - 1); memcpy(reinterpret_cast<void*>(lparam),text.data(),count * sizeof(wchar_t)); reinterpret_cast<wchar_t*>(lparam)[count] = 0; return count; }
    case WM_SETFONT: borrowed_font_ = reinterpret_cast<HFONT>(wparam); ApplyFont(); if (lparam) Invalidate(); return 0;
    case WM_GETFONT: return reinterpret_cast<LRESULT>(borrowed_font_ ? borrowed_font_ : GetStockObject(SYSTEM_FIXED_FONT));
    case EM_SETREADONLY: Call(SCI_SETREADONLY,wparam); ModifyStyle(ES_READONLY,wparam ? ES_READONLY : 0); return TRUE;
    case EM_GETMODIFY: return forced_modified_ || Call(SCI_GETMODIFY);
    case EM_SETMODIFY: forced_modified_ = wparam != 0; if (!wparam) Call(SCI_SETSAVEPOINT); return 0;
    case EM_SETLIMITTEXT: limit_ = wparam ? wparam : INT_MAX; return 0;
    case EM_CANUNDO: return Call(SCI_CANUNDO);
    case EM_UNDO: { const auto possible = Call(SCI_CANUNDO); Call(SCI_UNDO); return possible; }
    case EM_EMPTYUNDOBUFFER: Call(SCI_EMPTYUNDOBUFFER); return 0;
    case EM_GETSEL: { const auto start = UnitPosition(Call(SCI_GETSELECTIONSTART)), end = UnitPosition(Call(SCI_GETSELECTIONEND)); if (wparam) *reinterpret_cast<DWORD*>(wparam) = static_cast<DWORD>(start); if (lparam) *reinterpret_cast<DWORD*>(lparam) = static_cast<DWORD>(end); return start <= 65535 && end <= 65535 ? MAKELONG(start,end) : -1; }
    case EM_SETSEL: { const auto start = static_cast<std::intptr_t>(wparam), end = static_cast<std::intptr_t>(lparam); if (start < 0) Call(SCI_SETEMPTYSELECTION,Call(SCI_GETCURRENTPOS)); else Call(SCI_SETSEL,BytePosition(start),end < 0 ? Call(SCI_GETLENGTH) : BytePosition(end)); return 0; }
    case EM_REPLACESEL: { if (Call(SCI_GETREADONLY)) return 0; const std::wstring wide = lparam ? reinterpret_cast<const wchar_t*>(lparam) : L""; const auto selected = UnitPosition(Call(SCI_GETSELECTIONEND)) - UnitPosition(Call(SCI_GETSELECTIONSTART)); if (wide.size() + static_cast<std::size_t>(UnitPosition(Call(SCI_GETLENGTH)) - selected) > limit_) return 0; const auto value = ToUtf8(wide); if (!wide.empty() && value.empty()) return 0; if (!wparam) Call(SCI_SETUNDOCOLLECTION,0); Call(SCI_REPLACESEL,0,reinterpret_cast<LPARAM>(value.c_str())); if (!wparam) Call(SCI_SETUNDOCOLLECTION,1); return 0; }
    case EM_GETLINECOUNT: return Call(SCI_GETLINECOUNT);
    case EM_GETFIRSTVISIBLELINE: return Call(SCI_DOCLINEFROMVISIBLE,Call(SCI_GETFIRSTVISIBLELINE));
    case EM_LINESCROLL: Call(SCI_LINESCROLL,wparam,lparam); return TRUE;
    case EM_SCROLLCARET: Call(SCI_SCROLLCARET); return 0;
    case EM_LINEFROMCHAR: return Call(SCI_LINEFROMPOSITION,wparam == static_cast<WPARAM>(-1) ? Call(SCI_GETCURRENTPOS) : BytePosition(wparam));
    case EM_LINEINDEX: return UnitPosition(Call(SCI_POSITIONFROMLINE,wparam == static_cast<WPARAM>(-1) ? Call(SCI_LINEFROMPOSITION,Call(SCI_GETCURRENTPOS)) : wparam));
    case WM_COPY: Call(SCI_COPY); return 0; case WM_CUT: Call(SCI_CUT); return 0; case WM_PASTE: Call(SCI_PASTE); return 0; case WM_CLEAR: Call(SCI_CLEAR); return 0;
    case WM_GETDLGCODE: return DLGC_WANTALLKEYS | DLGC_WANTCHARS | DLGC_WANTARROWS | DLGC_HASSETSEL;
    default: return CEdit::WindowProc(message,wparam,lparam);
    }
}

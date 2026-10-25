#include "hex_view.h"
#include "resource.h"
#include "core/search.h"
#include "core/text_codec.h"
#include <cstring>

namespace {
constexpr std::size_t kClipboardLimit = 64 * 1024 * 1024;
UINT ByteClipboard() { static const UINT format = RegisterClipboardFormatW(L"SimpleEditorTool.Binary.v1"); return format; }
struct ClipboardScope { bool opened; explicit ClipboardScope(HWND window) : opened(OpenClipboard(window) != FALSE) {} ~ClipboardScope() { if (opened) CloseClipboard(); } };
bool SetClipboardBlock(UINT format, const void* data, std::size_t size) {
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE,size); if (!memory) return false;
    void* target = GlobalLock(memory); if (!target) { GlobalFree(memory); return false; }
    memcpy(target,data,size); GlobalUnlock(memory); if (!SetClipboardData(format,memory)) { GlobalFree(memory); return false; } return true;
}
std::wstring ReadClipboardText() {
    HGLOBAL memory = GetClipboardData(CF_UNICODETEXT); const auto bytes = memory ? GlobalSize(memory) : 0;
    if (!bytes || bytes > kClipboardLimit * 3) return L"";
    const auto* value = static_cast<const wchar_t*>(GlobalLock(memory)); if (!value) return L"";
    const auto count = bytes / sizeof(wchar_t); const auto* end = std::find(value,value + count,L'\0');
    std::wstring result(value,end); GlobalUnlock(memory); return result;
}
bool FlexibleHex(std::wstring text, std::vector<std::uint8_t>* bytes) {
    std::wstring normalized;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if ((text[i] == L'0' || text[i] == L'\\') && i + 1 < text.size() && (text[i + 1] == L'x' || text[i + 1] == L'X')) { i++; continue; }
        if (text[i] == L',' || text[i] == L';' || text[i] == L'-') normalized += L' '; else normalized += text[i];
    }
    return simple_editor::core::ParseHexBytes(normalized,bytes);
}
bool NativeClipboard(HWND window, bool writing, CHexView::ClipboardContent& content) {
    ClipboardScope scope(window); if (!scope.opened) return false;
    if (writing) {
        if (!EmptyClipboard()) return false;
        std::vector<std::uint8_t> binary(sizeof(std::uint64_t) + content.bytes.size()); const auto count = static_cast<std::uint64_t>(content.bytes.size());
        memcpy(binary.data(),&count,sizeof(count)); memcpy(binary.data() + sizeof(count),content.bytes.data(),content.bytes.size());
        return SetClipboardBlock(CF_UNICODETEXT,content.text.c_str(),(content.text.size() + 1) * sizeof(wchar_t)) && SetClipboardBlock(ByteClipboard(),binary.data(),binary.size());
    }
    if (!content.want_text && IsClipboardFormatAvailable(ByteClipboard())) {
        HGLOBAL memory = GetClipboardData(ByteClipboard()); const auto size = memory ? GlobalSize(memory) : 0; const auto* data = memory ? static_cast<const std::uint8_t*>(GlobalLock(memory)) : nullptr;
        if (data && size >= sizeof(std::uint64_t)) { std::uint64_t count = 0; memcpy(&count,data,sizeof(count)); if (count <= kClipboardLimit && count <= size - sizeof(count)) { content.bytes.assign(data + sizeof(count),data + sizeof(count) + static_cast<std::size_t>(count)); content.has_bytes = true; } }
        if (data) GlobalUnlock(memory);
    } else content.text = ReadClipboardText();
    return true;
}
}
void CHexView::NotifySelection() { if (selection_callback_) selection_callback_(); }
bool CHexView::HitTest(CPoint point, std::uint64_t* offset, bool* ascii, int* nibble) const {
    if (!document_ || !document_->IsOpen() || point.y < 0 || point.x + horizontal_offset_ < 8) return false;
    const int character = (point.x + horizontal_offset_ - 8) / char_width_;
    const int ascii_start = HexColumn(bytes_per_row_) + 2; int column = -1;
    *ascii = character >= ascii_start && character < ascii_start + static_cast<int>(bytes_per_row_); *nibble = 0;
    if (*ascii) column = character - ascii_start;
    else for (unsigned i = 0; i < bytes_per_row_; ++i) if (character >= HexColumn(i) && character < HexColumn(i) + 2) { column = static_cast<int>(i); *nibble = character - HexColumn(i); break; }
    if (column < 0) return false;
    *offset = (std::min)(document_->Size(),(top_row_ + point.y / row_height_) * bytes_per_row_ + column); return true;
}
void CHexView::OnLButtonDown(UINT flags, CPoint point) {
    if (SelectDecoded(point)) return; decoded_mode_ = false;
    SetFocus(); std::uint64_t hit; bool ascii; int nibble;
    if (!HitTest(point,&hit,&ascii,&nibble)) return;
    if (!(flags & MK_SHIFT)) anchor_ = hit;
    cursor_ = hit; drag_origin_ = hit; dragging_ = !(flags & MK_SHIFT); ascii_mode_ = ascii; nibble_ = nibble; highlight_enabled_ = true;
    if (dragging_) SetCapture(); Invalidate(); NotifySelection();
}
void CHexView::OnMouseMove(UINT flags, CPoint point) {
    if (!dragging_ || !(flags & MK_LBUTTON) || !document_) return;
    CRect r; GetClientRect(r); if (point.y < 0) { OnVScroll(SB_LINEUP,0,nullptr); point.y = 0; } else if (point.y >= r.Height()) { OnVScroll(SB_LINEDOWN,0,nullptr); point.y = (std::max)(0,r.Height() - 1); }
    std::uint64_t hit; bool ascii; int nibble; if (!HitTest(point,&hit,&ascii,&nibble)) return;
    if (hit >= document_->Size()) { cursor_ = document_->Size(); anchor_ = drag_origin_; }
    else if (hit >= drag_origin_) { anchor_ = drag_origin_; cursor_ = hit + 1; }
    else { anchor_ = (std::min)(document_->Size(),drag_origin_ + 1); cursor_ = hit; }
    Invalidate(); NotifySelection();
}
void CHexView::OnLButtonUp(UINT, CPoint) { if (GetCapture() == this) ReleaseCapture(); dragging_ = false; }
void CHexView::OnCaptureChanged(CWnd* window) { dragging_ = false; CWnd::OnCaptureChanged(window); }
void CHexView::SelectAll() { if (!document_) return; cursor_ = 0; anchor_ = document_->Size(); Invalidate(); NotifySelection(); }
bool CHexView::PutBytes(const std::vector<std::uint8_t>& bytes, bool insert) {
    if (read_only_ || !document_ || !document_->IsOpen() || bytes.empty()) return false;
    const auto start = SelectionStart(); const auto removed = insert ? SelectionLength() : (std::min<std::uint64_t>)(bytes.size(),document_->Size() - start);
    if (!document_->Replace(start,removed,bytes)) return false;
    cursor_ = anchor_ = start + bytes.size(); nibble_ = 0; if (change_callback_) change_callback_(); EnsureCursorVisible(); return true;
}
bool CHexView::DeleteSelection() {
    if (read_only_ || !document_ || !document_->IsOpen()) return false;
    const auto at = SelectionStart(), length = SelectionLength() ? SelectionLength() : (at < document_->Size() ? 1 : 0);
    if (!length || !document_->Delete(at,length)) return false;
    cursor_ = anchor_ = at; nibble_ = 0; if (change_callback_) change_callback_(); EnsureCursorVisible(); return true;
}
bool CHexView::Clipboard(UINT command) {
    if (!document_ || !document_->IsOpen()) return false;
    if (command == ID_HEX_SELECT_ALL) { SelectAll(); return true; }
    if (command == ID_HEX_READ_ONLY) { SetReadOnly(!read_only_); return true; }
    if (command == ID_HEX_INSERT_MODE) { if (!read_only_) SetInsertMode(!insert_mode_); return !read_only_; }
    const bool copying = command == ID_HEX_COPY || command == ID_HEX_COPY_TEXT || command == ID_HEX_CUT;
    if ((command == ID_HEX_CUT || !copying) && read_only_) return false;
    std::vector<std::uint8_t> bytes; std::wstring error;
    if (copying) {
        if (!SelectionLength()) { if (report) report(L"Select bytes first. Drag or use Shift+arrows."); return false; }
        if (SelectionLength() > kClipboardLimit || !document_->Read(SelectionStart(),static_cast<std::size_t>(SelectionLength()),&bytes)) { if (report) report(L"Clipboard operations support up to 64 MiB; use Extract range for larger selections."); return false; }
        std::wstring text;
        if (command == ID_HEX_COPY_TEXT) { simple_editor::core::TextCodec codec; if (!simple_editor::core::DecodeText(bytes,simple_editor::core::TextEncoding::Auto,&text,&codec,&error)) { if (report) report(error + L" Use Copy Hex for arbitrary binary data."); return false; } }
        else { static const wchar_t* digits = L"0123456789ABCDEF"; text.reserve(bytes.size() * 3); for (auto value : bytes) { if (!text.empty()) text += L' '; text += digits[value >> 4]; text += digits[value & 15]; } }
        ClipboardContent content; content.bytes = bytes; content.text = std::move(text); content.has_bytes = true;
        if (!(clipboard_access ? clipboard_access(true,content) : NativeClipboard(GetSafeHwnd(),true,content))) { if (report) report(L"Could not write clipboard data; clipboard may be busy."); return false; }
    } else {
        ClipboardContent content; content.want_text = command == ID_HEX_PASTE_TEXT;
        if (!(clipboard_access ? clipboard_access(false,content) : NativeClipboard(GetSafeHwnd(),false,content))) { if (report) report(L"Clipboard is currently busy."); return false; }
        if (content.has_bytes && !content.want_text) bytes = std::move(content.bytes);
        else if (command == ID_HEX_PASTE_TEXT) { simple_editor::core::TextCodec codec; codec.encoding = simple_editor::core::TextEncoding::Utf8; if (!simple_editor::core::EncodeText(content.text,codec,false,&bytes,&error)) { if (report) report(error); return false; } }
        else if (!FlexibleHex(content.text,&bytes)) { if (report) report(L"Clipboard must contain Hex pairs (00 FF, 0x00,0xFF or \\x00\\xFF). Use Paste UTF-8 text for ordinary text."); return false; }
    }
    if (command == ID_HEX_CUT) return DeleteSelection();
    if (copying) return true;
    if (bytes.size() > kClipboardLimit) { if (report) report(L"Clipboard operations support up to 64 MiB."); return false; }
    return PutBytes(bytes,command == ID_HEX_PASTE_INSERT || (command == ID_HEX_PASTE_TEXT && insert_mode_));
}
void CHexView::OnChar(UINT ch, UINT, UINT) {
    if (decoded_mode_) return;
    if (read_only_ || !document_ || !document_->IsOpen() || (GetKeyState(VK_CONTROL) & 0x8000) || ch < 32) return;
    if (ascii_mode_) {
        if (ch >= 0xD800 && ch <= 0xDBFF) { pending_surrogate_ = static_cast<wchar_t>(ch); return; }
        std::wstring text; if (pending_surrogate_ && ch >= 0xDC00 && ch <= 0xDFFF) text += pending_surrogate_; pending_surrogate_ = 0; text += static_cast<wchar_t>(ch);
        simple_editor::core::TextCodec codec; codec.encoding = simple_editor::core::TextEncoding::Utf8;
        std::vector<std::uint8_t> bytes; std::wstring error; if (simple_editor::core::EncodeText(text,codec,false,&bytes,&error)) PutBytes(bytes,insert_mode_); else if (report) report(error); return;
    }
    const int digit = ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : -1; if (digit < 0) return;
    if (nibble_ == 0 && (insert_mode_ || cursor_ == document_->Size() || SelectionLength())) {
        const auto at = SelectionStart(); const auto removed = insert_mode_ ? SelectionLength() : (std::min<std::uint64_t>)(1,document_->Size() - at);
        if (!document_->Replace(at,removed,{static_cast<std::uint8_t>(digit << 4)})) return; cursor_ = anchor_ = at; nibble_ = 1;
    } else {
        std::vector<std::uint8_t> value; if (cursor_ >= document_->Size() || !document_->Read(cursor_,1,&value)) return;
        if (!document_->SetByte(cursor_,static_cast<std::uint8_t>(nibble_ == 0 ? (value[0] & 15) | digit << 4 : (value[0] & 240) | digit))) return;
        if (++nibble_ == 2) { nibble_ = 0; ++cursor_; } anchor_ = cursor_;
    }
    if (change_callback_) change_callback_(); EnsureCursorVisible();
}
void CHexView::OnKeyDown(UINT ch, UINT repeat, UINT flags) {
    if (decoded_mode_ && ch != VK_TAB && ch != VK_LEFT && ch != VK_RIGHT && ch != VK_UP && ch != VK_DOWN && ch != VK_HOME && ch != VK_END && ch != VK_PRIOR && ch != VK_NEXT && ch != 'C') return;
    if (ch == VK_TAB) decoded_mode_ = false;
    if (!document_) return;
    const bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0, shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    if (control) { if (ch == 'A') SelectAll(); else if (ch == 'C') Clipboard(ID_HEX_COPY); else if (ch == 'X') Clipboard(ID_HEX_CUT); else if (ch == 'V') Clipboard(insert_mode_ ? ID_HEX_PASTE_INSERT : ID_HEX_PASTE_WRITE); else CWnd::OnKeyDown(ch,repeat,flags); return; }
    if (ch == VK_INSERT) { if (!read_only_) SetInsertMode(!insert_mode_); return; }
    if (ch == VK_DELETE) { DeleteSelection(); return; }
    if (ch == VK_BACK) { if (!SelectionLength() && !cursor_) return; if (!read_only_ && !SelectionLength()) SelectRange(cursor_ - 1,1); if (!read_only_) DeleteSelection(); return; }
    if (ch == VK_TAB) { ascii_mode_ = !ascii_mode_; nibble_ = 0; NotifySelection(); return; }
    const auto size = document_->Size();
    if (ch == VK_LEFT && cursor_) --cursor_; else if (ch == VK_RIGHT && cursor_ < size) ++cursor_;
    else if (ch == VK_UP) cursor_ = cursor_ >= bytes_per_row_ ? cursor_ - bytes_per_row_ : 0;
    else if (ch == VK_DOWN) cursor_ = (std::min)(size,cursor_ + bytes_per_row_);
    else if (ch == VK_PRIOR) cursor_ = cursor_ > VisibleRows() * static_cast<std::uint64_t>(bytes_per_row_) ? cursor_ - VisibleRows() * bytes_per_row_ : 0;
    else if (ch == VK_NEXT) cursor_ = (std::min)(size,cursor_ + VisibleRows() * bytes_per_row_);
    else if (ch == VK_HOME) cursor_ = 0; else if (ch == VK_END) cursor_ = size;
    else { CWnd::OnKeyDown(ch,repeat,flags); return; }
    if (!shift) anchor_ = cursor_; nibble_ = 0; highlight_enabled_ = true; EnsureCursorVisible();
}
void CHexView::OnContextMenu(CWnd*, CPoint point) {
    if (point.x == -1) { point = CPoint(12,12); ClientToScreen(&point); }
    CMenu menu; menu.CreatePopupMenu(); const UINT disabled = read_only_ ? MF_GRAYED : 0;
    menu.AppendMenu(MF_STRING,ID_HEX_COPY,L"Copy Hex\tCtrl+C"); menu.AppendMenu(MF_STRING,ID_HEX_COPY_TEXT,L"Copy decoded text");
    menu.AppendMenu(MF_STRING | disabled,ID_HEX_CUT,L"Cut\tCtrl+X"); menu.AppendMenu(MF_STRING | disabled,ID_HEX_PASTE_WRITE,L"Paste overwrite"); menu.AppendMenu(MF_STRING | disabled,ID_HEX_PASTE_INSERT,L"Paste insert"); menu.AppendMenu(MF_STRING | disabled,ID_HEX_PASTE_TEXT,L"Paste UTF-8 text");
    menu.AppendMenu(MF_SEPARATOR); menu.AppendMenu(MF_STRING,ID_HEX_SELECT_ALL,L"Select all\tCtrl+A"); menu.AppendMenu(MF_STRING | disabled | (insert_mode_ ? MF_CHECKED : 0),ID_HEX_INSERT_MODE,L"Insert mode\tIns");
    const HWND identity = GetSafeHwnd(); const auto command = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_NONOTIFY,point.x,point.y,this);
    if (::IsWindow(identity) && CWnd::FromHandlePermanent(identity) == this && command) Clipboard(command);
}

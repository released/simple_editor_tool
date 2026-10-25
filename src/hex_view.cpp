#include "hex_view.h"
#include "resource.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

BEGIN_MESSAGE_MAP(CHexView, CWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_VSCROLL()
    ON_WM_HSCROLL()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_CAPTURECHANGED()
    ON_WM_CONTEXTMENU()
    ON_WM_CHAR()
    ON_WM_KEYDOWN()
    ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

CHexView::CHexView() : document_(nullptr), top_row_(0), cursor_(0), nibble_(0), read_only_(false) {}
int CHexView::OnCreate(LPCREATESTRUCT create) {
    if (CWnd::OnCreate(create) == -1) return -1;
    if (!SetZoomPercent(100)) return -1;
    ModifyStyle(0, WS_HSCROLL); return 0;
}
bool CHexView::SetZoomPercent(int percent) {
    if (!GetSafeHwnd()) return false;
    percent = (std::clamp)(percent,50,300);
    if (percent == zoom_percent_ && font_.GetSafeHandle()) return true;
    CClientDC dc(this); CFont replacement;
    if (!replacement.CreatePointFont(MulDiv(font_tenths_,percent,100),font_face_.c_str(),&dc)) return false;
    auto* old = dc.SelectObject(&replacement); TEXTMETRIC metrics{}; dc.GetTextMetrics(&metrics); dc.SelectObject(old);
    CFont previous; previous.Attach(font_.Detach()); font_.Attach(replacement.Detach());
    char_width_ = (std::max)(1L,metrics.tmAveCharWidth); row_height_ = (std::max)(1L,metrics.tmHeight + MulDiv(3,percent,100));
    zoom_percent_ = percent; UpdateScroll(); Invalidate(); return true;
}
bool CHexView::SetAppearance(const std::wstring& face,unsigned font_tenths,unsigned theme) {
    if (face.empty() || face.size() > 31 || font_tenths < 60 || font_tenths > 320 || theme > 2) return false;
    const auto old_face = font_face_; const auto old_size = font_tenths_,old_theme = theme_; const int percent = zoom_percent_;
    font_face_ = face; font_tenths_ = font_tenths; theme_ = theme; zoom_percent_ = -1;
    if (SetZoomPercent(percent)) return true; font_face_ = old_face; font_tenths_ = old_size; theme_ = old_theme; zoom_percent_ = percent; return false;
}
void CHexView::SetDocument(simple_editor::core::BinaryDocument* document) {
    document_ = document; top_row_ = cursor_ = anchor_ = 0; nibble_ = 0;
    if (document_ && base_address_ > UINT64_MAX - document_->Size()) base_address_ = 0;
    horizontal_offset_ = 0; UpdateScroll(); Invalidate(); NotifySelection();
}
unsigned CHexView::VisibleRows() const { CRect r; GetClientRect(r); return (std::max)(1,r.Height() / row_height_); }
int CHexView::AddressDigits() const {
    const auto size = document_ ? document_->Size() : 0;
    return base_address_ > 0xFFFFFFFFULL || size > 0xFFFFFFFFULL - (std::min<std::uint64_t>)(base_address_,0xFFFFFFFFULL) ? 16 : 8;
}
int CHexView::HexColumn(int column) const {
    return AddressDigits() + 2 + column * 3 + (grouping_ > 1 ? (std::min)(column,static_cast<int>(bytes_per_row_) - 1) / static_cast<int>(grouping_) : 0);
}
bool CHexView::SetLayout(unsigned bytes_per_row, unsigned grouping, std::uint64_t base_address) {
    if ((bytes_per_row != 8 && bytes_per_row != 16 && bytes_per_row != 32) || (grouping != 1 && grouping != 2 && grouping != 4 && grouping != 8) ||
        (document_ && base_address > UINT64_MAX - document_->Size())) return false;
    const auto top_address = top_row_ * bytes_per_row_; bytes_per_row_ = bytes_per_row; grouping_ = grouping; base_address_ = base_address;
    top_row_ = top_address / bytes_per_row_; UpdateScroll(); Invalidate(); NotifySelection(); return true;
}
void CHexView::UpdateScroll() {
    if (!GetSafeHwnd()) return;
    const std::uint64_t rows = document_ ? document_->Size() / bytes_per_row_ + 1 : 1;
    const auto max_top = rows > VisibleRows() ? rows - VisibleRows() : 0; top_row_ = (std::min)(top_row_,max_top);
    constexpr int range = 1000000; SCROLLINFO si{}; si.cbSize = sizeof(si); si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMax = range; si.nPage = static_cast<UINT>((std::max)(1.0,(std::min)(static_cast<double>(range + 1),static_cast<double>(VisibleRows()) * (range + 1) / rows)));
    const int travel = range - static_cast<int>(si.nPage) + 1;
    si.nPos = max_top ? static_cast<int>(static_cast<long double>(top_row_) * travel / max_top) : 0; SetScrollInfo(SB_VERT,&si,TRUE);
    CRect r; GetClientRect(r); const int content_width = 16 + (HexColumn(bytes_per_row_) + bytes_per_row_ + 3 + (decoding_ == simple_editor::core::HexDecoding::None ? 0 : bytes_per_row_ + 3)) * char_width_;
    horizontal_offset_ = (std::clamp)(horizontal_offset_,0,(std::max)(0,content_width - r.Width()));
    SCROLLINFO hs{}; hs.cbSize = sizeof(hs); hs.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    hs.nMax = content_width - 1; hs.nPage = (std::max)(0,r.Width()); hs.nPos = horizontal_offset_; SetScrollInfo(SB_HORZ,&hs,TRUE);
}
void CHexView::OnPaint() {
    CPaintDC dc(this); CRect client; GetClientRect(client); dc.FillSolidRect(client,theme_ == 2 ? GetSysColor(COLOR_WINDOW) : theme_ == 1 ? RGB(30,30,30) : RGB(255,255,255));
    auto* old = dc.SelectObject(&font_); dc.SetBkMode(TRANSPARENT); dc.SetTextColor(theme_ == 2 ? GetSysColor(COLOR_WINDOWTEXT) : theme_ == 1 ? RGB(225,225,225) : RGB(30,35,42));
    if (!document_ || !document_->IsOpen()) { dc.TextOutW(12,12,read_only_ ? L"Choose a comparison file" : L"Drop a file here or choose File > Open"); dc.SelectObject(old); return; }
    const int origin = 8 - horizontal_offset_; const unsigned rows = VisibleRows(); std::vector<std::uint8_t> viewport;
    if (!document_->Read(top_row_ * bytes_per_row_,static_cast<std::size_t>(rows) * bytes_per_row_,&viewport)) {
        dc.TextOutW(origin,0,L"Source changed or read failed. Reload with Ctrl+R."); dc.SelectObject(old); return;
    }
    const auto selected_start = SelectionStart(), selected_length = SelectionLength();
    const bool show_bookmarks = bookmarks && bookmarks->Matches(*document_);
    std::vector<simple_editor::core::DecodedGlyph> decoded; if (decoding_ != simple_editor::core::HexDecoding::None) { const auto start = top_row_ * bytes_per_row_; std::wstring error; simple_editor::core::DecodeHexWindow(*document_,start,viewport.size(),decoding_,&decoded,&error); }
    for (unsigned row = 0; row < rows; ++row) {
        const auto offset = (top_row_ + row) * bytes_per_row_; if (offset > document_->Size()) break;
        const auto begin = static_cast<std::size_t>(row) * bytes_per_row_;
        const auto count = begin < viewport.size() ? (std::min<std::size_t>)(bytes_per_row_,viewport.size() - begin) : 0;
        std::wostringstream address; address << std::uppercase << std::hex << std::setfill(L'0') << std::setw(AddressDigits()) << offset + base_address_;
        dc.SetTextColor(theme_ == 2 ? GetSysColor(COLOR_WINDOWTEXT) : theme_ == 1 ? RGB(225,225,225) : RGB(30,35,42));
        dc.TextOutW(origin,row * row_height_,address.str().c_str());
        const int ascii_column = HexColumn(bytes_per_row_) + 2;
        for (unsigned column = 0; column < bytes_per_row_; ++column) {
            const auto at = offset + column; const bool selected = selected_length && at >= selected_start && at - selected_start < selected_length;
            const bool caret = !selected_length && highlight_enabled_ && at == cursor_;
            if (modified_ranges && !selected && !caret) { const auto found = std::upper_bound(modified_ranges->begin(),modified_ranges->end(),at,[](auto value,const auto& range) { return value < range.offset; }); if (found != modified_ranges->begin()) { const auto& range = *std::prev(found); if (at >= range.offset && at - range.offset < (std::max<std::uint64_t>)(1,range.length)) dc.FillSolidRect(origin + HexColumn(column) * char_width_,row * row_height_,2 * char_width_,row_height_,theme_ == 1 ? RGB(100,65,20) : RGB(255,225,130)); } }
            if (show_bookmarks && !selected && !caret) for (const auto& range : bookmarks->ranges) if (!range.stale && at >= range.offset && at - range.offset < range.length) { const auto c = range.color; const auto color = theme_ == 1 ? RGB(GetRValue(c) / 3,GetGValue(c) / 3,GetBValue(c) / 3) : RGB((GetRValue(c) + 255) / 2,(GetGValue(c) + 255) / 2,(GetBValue(c) + 255) / 2); dc.FillSolidRect(origin + HexColumn(column) * char_width_,row * row_height_,char_width_ * 2,row_height_,color); break; }
            if (selected || caret) { const auto color = theme_ == 2 ? GetSysColor(COLOR_HIGHLIGHT) : theme_ == 1 ? RGB(45,75,115) : RGB(185,215,250); dc.FillSolidRect(origin + HexColumn(column) * char_width_,row * row_height_,char_width_ * 2,row_height_,color); dc.FillSolidRect(origin + (ascii_column + column) * char_width_,row * row_height_,char_width_,row_height_,color); }
            if (column < count) {
                dc.SetTextColor(theme_ == 2 ? GetSysColor(selected || caret ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT) : theme_ == 1 ? RGB(225,225,225) : RGB(30,35,42));
                wchar_t byte[4]{}; swprintf_s(byte,L"%02X",viewport[begin + column]); dc.TextOutW(origin + HexColumn(column) * char_width_,row * row_height_,byte);
                const wchar_t value = viewport[begin + column] >= 32 && viewport[begin + column] < 127 ? viewport[begin + column] : L'.';
                dc.TextOutW(origin + (ascii_column + column) * char_width_,row * row_height_,&value,1);
            }
        }
    }
    if (!decoded.empty()) for (const auto& glyph : decoded) { const auto first = (std::max)(glyph.offset,top_row_ * bytes_per_row_); const auto last = glyph.offset + glyph.length; for (auto position = first; position < last;) { const auto relative = position - top_row_ * bytes_per_row_; const auto row = relative / bytes_per_row_,column = relative % bytes_per_row_; if (row >= rows) break; const auto span = (std::min<std::uint64_t>)(last - position,bytes_per_row_ - column); const int x = origin + (HexColumn(bytes_per_row_) + 2 + bytes_per_row_ + 3 + static_cast<int>(column)) * char_width_,y = static_cast<int>(row) * row_height_; CRect clip(x,y,x + (std::max)(static_cast<int>(span) * char_width_,position == glyph.offset ? static_cast<int>(dc.GetTextExtent(glyph.text.c_str()).cx) : char_width_),y + row_height_); const auto text = position == glyph.offset ? glyph.text : L"\x00B7"; const bool selected = selected_length && glyph.offset < selected_start + selected_length && glyph.offset + glyph.length > selected_start; if (selected) dc.FillSolidRect(clip,theme_ == 1 ? RGB(45,75,115) : RGB(185,215,250)); dc.SetTextColor(theme_ == 1 ? RGB(225,225,225) : RGB(30,35,42)); dc.ExtTextOutW(x,y,ETO_CLIPPED,clip,text.c_str(),static_cast<UINT>(text.size()),nullptr); position += span; } }
    dc.SelectObject(old);
}
void CHexView::OnSize(UINT type, int cx, int cy) { CWnd::OnSize(type,cx,cy); UpdateScroll(); Invalidate(TRUE); }
void CHexView::OnVScroll(UINT code, UINT, CScrollBar*) {
    const auto previous_top = top_row_;
    const auto rows = document_ ? document_->Size() / bytes_per_row_ + 1 : 1;
    const auto max_top = rows > VisibleRows() ? rows - VisibleRows() : 0;
    if (code == SB_LINEUP && top_row_) --top_row_;
    else if (code == SB_LINEDOWN && top_row_ < max_top) ++top_row_;
    else if (code == SB_PAGEUP) top_row_ = top_row_ > VisibleRows() ? top_row_ - VisibleRows() : 0;
    else if (code == SB_PAGEDOWN) top_row_ = (std::min)(max_top,top_row_ + VisibleRows());
    else if (code == SB_TOP) top_row_ = 0; else if (code == SB_BOTTOM) top_row_ = max_top;
    else if (code == SB_THUMBTRACK || code == SB_THUMBPOSITION) {
        SCROLLINFO info{}; info.cbSize = sizeof(info); info.fMask = SIF_ALL; GetScrollInfo(SB_VERT,&info);
        const int travel = info.nMax - static_cast<int>(info.nPage) + 1; top_row_ = travel > 0 ? static_cast<std::uint64_t>(static_cast<long double>(info.nTrackPos) * max_top / travel) : 0;
    }
    if (top_row_ == previous_top) return;
    UpdateScroll(); Invalidate(FALSE); if (scroll_callback_) scroll_callback_(top_row_);
}
void CHexView::OnHScroll(UINT code, UINT, CScrollBar*) {
    if (code == SB_LINELEFT) horizontal_offset_ -= char_width_; else if (code == SB_LINERIGHT) horizontal_offset_ += char_width_;
    else if (code == SB_PAGELEFT) horizontal_offset_ -= char_width_ * 16; else if (code == SB_PAGERIGHT) horizontal_offset_ += char_width_ * 16;
    else if (code == SB_THUMBTRACK || code == SB_THUMBPOSITION) { SCROLLINFO info{}; info.cbSize = sizeof(info); info.fMask = SIF_TRACKPOS; GetScrollInfo(SB_HORZ,&info); horizontal_offset_ = info.nTrackPos; }
    UpdateScroll(); Invalidate();
}
BOOL CHexView::OnMouseWheel(UINT, short delta, CPoint) { for (int i = 0; i < 3; ++i) OnVScroll(delta > 0 ? SB_LINEUP : SB_LINEDOWN,0,nullptr); return TRUE; }
void CHexView::EnsureCursorVisible() {
    const auto row = cursor_ / bytes_per_row_; if (row < top_row_) top_row_ = row; else if (row >= top_row_ + VisibleRows()) top_row_ = row - VisibleRows() + 1;
    UpdateScroll(); Invalidate(); if (scroll_callback_) scroll_callback_(top_row_); NotifySelection();
}
void CHexView::GoTo(std::uint64_t offset) { if (!document_) return; decoded_mode_ = false; cursor_ = anchor_ = (std::min)(offset,document_->Size()); nibble_ = 0; highlight_enabled_ = true; top_row_ = cursor_ / bytes_per_row_; UpdateScroll(); Invalidate(); NotifySelection(); }
std::uint64_t CHexView::CursorOffset() const { return cursor_; }
void CHexView::SetReadOnly(bool value) { read_only_ = value; NotifySelection(); }
void CHexView::SetTopRow(std::uint64_t row, bool notify) { top_row_ = row; UpdateScroll(); Invalidate(); if (notify && scroll_callback_) scroll_callback_(top_row_); }
void CHexView::SetScrollCallback(const std::function<void(std::uint64_t)>& callback) { scroll_callback_ = callback; }
void CHexView::SetChangeCallback(const std::function<void()>& callback) { change_callback_ = callback; }
void CHexView::SelectRange(std::uint64_t offset, std::uint64_t length) { GoTo(offset); anchor_ = document_ ? cursor_ + (std::min)(length,document_->Size() - cursor_) : cursor_; Invalidate(); NotifySelection(); }
void CHexView::ClearSelection() { anchor_ = cursor_; highlight_enabled_ = false; Invalidate(); NotifySelection(); }


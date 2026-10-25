#pragma once

#include <afxwin.h>
#include <cstdint>
#include <algorithm>
#include <functional>
#include "core/binary_document.h"
#include "core/hex_bookmarks.h"
#include "core/binary_workflows.h"

class CHexView : public CWnd {
public:
    struct ClipboardContent { std::vector<std::uint8_t> bytes; std::wstring text; bool has_bytes = false, want_text = false; };
    std::function<bool(bool, ClipboardContent&)> clipboard_access;
    CHexView();
    void SetDocument(simple_editor::core::BinaryDocument* document);
    void GoTo(std::uint64_t offset);
    std::uint64_t CursorOffset() const;
    void SetReadOnly(bool read_only);
    void SetTopRow(std::uint64_t row, bool notify = false);
    void SetScrollCallback(const std::function<void(std::uint64_t)>& callback);
    void SetChangeCallback(const std::function<void()>& callback);
    void SelectRange(std::uint64_t offset, std::uint64_t length);
    void ClearSelection();
    std::uint64_t SelectionStart() const { return (std::min)(anchor_,cursor_); }
    std::uint64_t SelectionLength() const { return anchor_ > cursor_ ? anchor_ - cursor_ : cursor_ - anchor_; }
    bool Clipboard(UINT command);
    bool DeleteSelection();
    void SelectAll();
    bool InsertMode() const { return insert_mode_; }
    bool AsciiMode() const { return ascii_mode_; }
    bool ReadOnly() const { return read_only_; }
    const simple_editor::core::HexBookmarks* bookmarks = nullptr;
    const std::vector<simple_editor::core::ModifiedRange>* modified_ranges = nullptr;
    void SetDecoding(simple_editor::core::HexDecoding value) { decoding_ = value; UpdateScroll(); Invalidate(); NotifySelection(); }
    simple_editor::core::HexDecoding Decoding() const { return decoding_; }
    void SetInsertMode(bool value) { insert_mode_ = value; NotifySelection(); }
    bool SetLayout(unsigned bytes_per_row, unsigned grouping, std::uint64_t base_address);
    unsigned BytesPerRow() const { return bytes_per_row_; }
    unsigned Grouping() const { return grouping_; }
    std::uint64_t BaseAddress() const { return base_address_; }
    std::uint64_t DocumentSize() const { return document_ ? document_->Size() : 0; }
    void SetSelectionCallback(std::function<void()> callback) { selection_callback_ = std::move(callback); }
    std::function<void(const std::wstring&)> report;
    std::uint64_t TopRow() const { return top_row_; }
    bool SetZoomPercent(int percent);
    bool SetAppearance(const std::wstring& face,unsigned font_tenths,unsigned theme);
    friend class NativeUiTest;

protected:
    afx_msg void OnPaint();
    afx_msg int OnCreate(LPCREATESTRUCT create);
    afx_msg void OnHScroll(UINT code, UINT pos, CScrollBar* bar);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg void OnVScroll(UINT code, UINT pos, CScrollBar* bar);
    afx_msg void OnLButtonDown(UINT flags, CPoint point);
    afx_msg void OnLButtonUp(UINT flags, CPoint point);
    afx_msg void OnMouseMove(UINT flags, CPoint point);
    afx_msg void OnCaptureChanged(CWnd* window);
    afx_msg void OnContextMenu(CWnd*, CPoint point);
    afx_msg void OnChar(UINT character, UINT repeat, UINT flags);
    afx_msg void OnKeyDown(UINT character, UINT repeat, UINT flags);
    afx_msg BOOL OnMouseWheel(UINT flags, short delta, CPoint point);
    DECLARE_MESSAGE_MAP()

private:
    void UpdateScroll();
    int HexColumn(int column) const;
    void EnsureCursorVisible();
    unsigned int VisibleRows() const;
    int AddressDigits() const;
    bool HitTest(CPoint point, std::uint64_t* offset, bool* ascii, int* nibble) const;
    bool SelectDecoded(CPoint point);
    void NotifySelection();
    bool PutBytes(const std::vector<std::uint8_t>& bytes, bool insert);
    simple_editor::core::BinaryDocument* document_;
    std::uint64_t top_row_;
    std::uint64_t cursor_;
    int nibble_;
    bool read_only_;
    std::function<void(std::uint64_t)> scroll_callback_;
    std::function<void()> change_callback_;
    CFont font_;
    int char_width_ = 8;
    int row_height_ = 20;
    int horizontal_offset_ = 0;
    int zoom_percent_ = 100;
    std::wstring font_face_ = L"Consolas";
    unsigned font_tenths_ = 100,theme_ = 1;
    std::uint64_t anchor_ = 0, drag_origin_ = 0, base_address_ = 0;
    bool dragging_ = false, ascii_mode_ = false, insert_mode_ = false;
    unsigned bytes_per_row_ = 16, grouping_ = 8;
    wchar_t pending_surrogate_ = 0;
    std::function<void()> selection_callback_;
    bool highlight_enabled_ = true;
    bool decoded_mode_ = false;
    simple_editor::core::HexDecoding decoding_ = simple_editor::core::HexDecoding::None;
};


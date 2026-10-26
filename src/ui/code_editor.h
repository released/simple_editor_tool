#pragma once
#include <afxwin.h>
#include <functional>
#include <cstdint>
#include <string>
#include "../core/unicode_notices.h"
#include "../../vendor/scintilla/include/Scintilla.h"
struct IDropTarget;

class CCodeEditor : public CEdit {
public:
    ~CCodeEditor() override;
    BOOL Create(DWORD style, const RECT& bounds, CWnd* parent, UINT id);
    LRESULT Call(UINT message, WPARAM wparam = 0, LPARAM lparam = 0) const { return ::SendMessageW(GetSafeHwnd(),message,wparam,lparam); }
    void ConfigureLanguage(const std::wstring& path,const std::wstring& mode = L"");
    std::string SyntaxName() const { return lexer_; }
    void SetDark(bool dark);
    void SetTheme(unsigned theme);
    void SetWrap(bool wrap);
    void SetWhitespace(bool visible);
    void SetTabWidth(unsigned width, bool use_tabs);
    void ToggleBookmark();
    void NextBookmark(bool previous = false);
    bool CanRedo() const { return Call(SCI_CANREDO) != 0; }
    void Redo() { Call(SCI_REDO); }
    void GoToLine(std::uint64_t line);
    std::wstring Status() const;
    std::string Utf8() const;
    std::string Keywords() const { return words_; }
    std::uint64_t Generation() const { return generation_; }
    void ReplaceGenerated(const std::wstring& text, int undo_token);
    void ClearMarks();
    void ShowUnicodeNotices(const simple_editor::core::UnicodeNotices&,const std::set<std::uint32_t>& allowed);
    void ClearUnicodeNotices();
    void SetSourcePresentation(bool enabled);
    bool SourcePresentation() const { return source_presentation_; }
    std::intptr_t ContentStart() const;
    std::intptr_t NativePosition(std::intptr_t content) const;
    std::intptr_t ContentPosition(std::intptr_t native) const;
    std::intptr_t NativeLine(std::intptr_t original) const { return original + (source_presentation_ ? 1 : 0); }
    std::intptr_t OriginalLine(std::intptr_t native) const;
    std::function<void()> selection_changed;
    std::function<void(int,bool)> undo_container;
    static std::string ToUtf8(const std::wstring& value);
    static std::wstring FromUtf8(const std::string& value);
    static void ReleaseResources();
    static bool InitializeResources();
    friend class NativeUiTest;
protected:
    LRESULT WindowProc(UINT message, WPARAM wparam, LPARAM lparam) override;
    afx_msg void OnNotification(NMHDR* header, LRESULT* result);
    DECLARE_MESSAGE_MAP()
private:
    void UpdateMargins();
    void ApplyFont();
    void InitializeDropTarget();
    void DetachDropTarget();
    bool SourceMessage(UINT,WPARAM,LPARAM,LRESULT*);
    bool source_presentation_ = false;
    std::intptr_t source_margin_count_ = -1;
    std::intptr_t BytePosition(std::intptr_t units) const;
    std::intptr_t UnitPosition(std::intptr_t bytes) const;
    HFONT borrowed_font_ = nullptr;
    bool suppress_ = false, forced_modified_ = false, dark_ = false;
    unsigned theme_ = 0;
    std::size_t limit_ = 128 * 1024 * 1024;
    std::uint64_t generation_ = 0;
    IDropTarget* drop_target_ = nullptr;
    std::string lexer_, words_;
};

#pragma once
#include <afxwin.h>
#include <functional>
#include <cstdint>
#include <string>
#include <vector>

enum class SearchAction { FindNext, Replace, ReplaceAll, Count, FindAll };
struct SearchRequest {
    CString find;
    CString replacement;
    bool hex = false;
    int mode = 0;
    bool match_case = true, whole_word = false, previous = false, selection_only = false, wrap = true, all_open = false, preview = false;
    SearchAction action = SearchAction::FindNext;
    int mark_colour = -1;
};

class CSearchDialog : public CDialog {
public:
    CSearchDialog();
    std::function<void(const SearchRequest&)> execute;
    std::function<void()> cancel;
    bool allow_all_open = false;
    void Show(bool replace, bool hex, CWnd* owner);
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    void OnCancel() override;
    afx_msg void OnReplace();
    afx_msg void OnReplaceAll();
    afx_msg void OnCount();
    afx_msg void OnFindAll();
    afx_msg void OnPrevious();
    afx_msg void OnScopeChanged();
    afx_msg void OnPreview();
    afx_msg void OnMultiline();
    DECLARE_MESSAGE_MAP()
private:
    void Dispatch(SearchAction action, bool previous = false, bool preview = false);
    void InitializeMultiline();
    CEdit multiline_find_,multiline_replacement_;
    std::vector<std::pair<HWND,CRect>> original_bounds_;
    CRect original_window_;
    bool multiline_ = false;
};

class CAddressDialog : public CDialog {
public:
    explicit CAddressDialog(std::uint64_t limit, CWnd* owner);
    std::uint64_t address = 0;
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
private:
    std::uint64_t limit_;
};

class CValueDialog : public CDialog {
public:
    CValueDialog(CWnd* owner, const std::wstring& title, const std::wstring& prompt, const std::wstring& value);
    std::wstring value;
    std::function<bool(const std::wstring&,std::wstring*)> validate;
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
private:
    std::wstring title_, prompt_;
};

class CNativeNoticesDialog : public CDialog {
public:
    explicit CNativeNoticesDialog(CWnd* parent);
protected:
    BOOL OnInitDialog() override;
};
class CTextResultDialog : public CDialog {
public:
    CTextResultDialog(CWnd* parent, std::wstring title, std::wstring text);
protected:
    BOOL OnInitDialog() override;
private:
    std::wstring title_, text_;
};
class CTextPreviewDialog : public CDialog {
public:
    CTextPreviewDialog(CWnd* parent,std::wstring text);
protected:
    BOOL OnInitDialog() override;
private:
    std::wstring text_;
};

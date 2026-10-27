#pragma once
#include "code_editor.h"
#include "../core/function_outline.h"
#include <afxcmn.h>
#include <future>
#include <climits>
class CFunctionSidebar : public CWnd {
public:
    bool Create(CWnd* owner);
    ~CFunctionSidebar() override;
    void Tick(CCodeEditor&,std::uint64_t,const std::wstring&,simple_editor::core::OutlineLanguage,bool start,unsigned theme);
    void InvalidateSource();
    void RefreshNow();
    void FocusList() { if (list_.GetSafeHwnd()) list_.SetFocus(); }
    void Stop();
    bool Current(const CCodeEditor&,std::uint64_t,const std::wstring&,simple_editor::core::OutlineLanguage) const;
    std::function<void(std::size_t)> navigate;
    friend class NativeUiTest;
protected:
    afx_msg int OnCreate(LPCREATESTRUCT);
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnDestroy();
    afx_msg void OnDisplay(NMHDR*,LRESULT*);
    afx_msg void OnActivate(NMHDR*,LRESULT*);
    afx_msg void OnKey(NMHDR*,LRESULT*);
    DECLARE_MESSAGE_MAP()
private:
    struct Result { bool ok = false,truncated = false; std::uint64_t generation = 0,revision = 0; std::wstring path; simple_editor::core::OutlineLanguage language = simple_editor::core::OutlineLanguage::None; std::vector<simple_editor::core::MarkdownHeading> entries; };
    void Status(const std::wstring&);
    void Layout();
    CFont ui_font_;
    CStatic caption_,status_;
    CListCtrl list_;
    std::future<Result> job_;
    std::atomic_bool cancelled_{false};
    std::vector<simple_editor::core::MarkdownHeading> entries_;
    std::uint64_t generation_ = UINT64_MAX,revision_ = UINT64_MAX;
    std::wstring path_;
    simple_editor::core::OutlineLanguage language_ = simple_editor::core::OutlineLanguage::None;
    ULONGLONG changed_at_ = 0;
    bool ready_ = false,failed_ = false;
    unsigned theme_ = UINT_MAX;
};

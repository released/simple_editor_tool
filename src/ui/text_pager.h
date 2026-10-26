#pragma once
#include "code_editor.h"
#include "../core/text_page.h"
#include <future>
class CTextPager : public CDialog {
public:
    CTextPager(CWnd* owner,std::unique_ptr<simple_editor::core::BinaryDocument> source,simple_editor::core::TextEncoding encoding);
    ~CTextPager() override;
    friend class NativeUiTest;
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM,LPARAM) override;
    BOOL PreTranslateMessage(MSG*) override;
    void OnOK() override {}
    void OnCancel() override;
    afx_msg void OnTimer(UINT_PTR);
    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*);
    DECLARE_MESSAGE_MAP()
private:
    bool Load(std::uint64_t offset,bool known,int direction);
    void Poll();
    void Zoom(int delta);
    void Layout();
    void FindDialog();
    bool FindNext();
    std::unique_ptr<simple_editor::core::BinaryDocument> source_;
    simple_editor::core::TextEncoding encoding_;
    struct Result { simple_editor::core::TextPage page; std::wstring error; bool searching = false; std::uint64_t found = UINT64_MAX,after = 0; };
    std::future<Result> job_;
    std::atomic_bool cancelled_{false};
    simple_editor::core::TextPage page_;
    CCodeEditor editor_;
    std::vector<std::uint64_t> previous_;
    std::uint64_t old_begin_ = 0;
    int direction_ = 0, zoom_ = 100;
    bool closing_ = false;
    std::wstring query_;
    std::wstring ui_language_;
    std::uint64_t next_match_ = 0;
    bool select_match_ = false;
};

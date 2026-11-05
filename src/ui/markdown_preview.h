#pragma once
#include "code_editor.h"
#include "../core/markdown_preview.h"
#include <future>
#include <climits>
class CMarkdownPreview : public CWnd {
public:
    bool Create(CWnd*);
    ~CMarkdownPreview() override;
    void Tick(CCodeEditor&,std::uint64_t,const std::wstring&,bool visible,unsigned theme,int font_tenths,int zoom);
    void Stop();
    bool RouteKey(MSG*);
    bool Current(const CCodeEditor&,std::uint64_t) const;
    void SynchronizeScroll(CCodeEditor&,std::uint64_t);
    friend class NativeUiTest;
protected:
    afx_msg int OnCreate(LPCREATESTRUCT);
    afx_msg void OnSize(UINT,int,int);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()
private:
    void Present(const simple_editor::core::MarkdownPreview&,unsigned);
    void BuildScrollMap(const simple_editor::core::MarkdownPreview&);
    struct ScrollAnchor { std::size_t source_line; LONG character; };
    std::vector<ScrollAnchor> scroll_anchors_;
    std::vector<LONG> text_positions_;
    std::size_t source_lines_ = 0;
    bool truncated_ = false,sync_ready_ = false;
    LRESULT source_top_ = -1,preview_top_ = -1;
    CStatic caption_;
    CWnd rich_;
    CFont font_;
    HMODULE library_ = nullptr;
    std::future<simple_editor::core::MarkdownPreview> job_;
    std::atomic_bool cancelled_{false};
    std::wstring requested_,running_,applied_;
    std::wstring caption_text_;
    std::uint64_t generation_ = UINT64_MAX,revision_ = UINT64_MAX;
    ULONGLONG due_ = 0;
};

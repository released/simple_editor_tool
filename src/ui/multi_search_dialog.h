#pragma once
#include <afxcmn.h>
#include <afxdlgs.h>
#include "../services/multi_file_search.h"
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
class CMultiSearchDialog : public CDialog {
public:
    ~CMultiSearchDialog() override;
    void Show(CWnd* owner);
    bool Present(CWnd* owner,std::vector<simple_editor::services::MultiSearchMatch> matches,const std::wstring& status);
    bool Busy() const { return job_.valid(); }
    std::function<bool(int,std::vector<simple_editor::services::MultiSearchSource>*,std::wstring*)> capture;
    std::function<void(const simple_editor::services::MultiSearchMatch&)> navigate;
    friend class NativeUiTest;
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM,LPARAM) override;
    void OnOK() override;
    void OnCancel() override;
    afx_msg void OnTimer(UINT_PTR id);
    afx_msg void OnDestroy();
    afx_msg void OnDisplay(NMHDR*,LRESULT*);
    afx_msg void OnActivate(NMHDR*,LRESULT*);
    DECLARE_MESSAGE_MAP()
private:
    bool Start(simple_editor::services::MultiSearchRequest request,const std::filesystem::path& folder,std::vector<simple_editor::services::MultiSearchSource> sources);
    void Stop();
    void Tick();
    void EnableInputs(bool enabled);
    CListCtrl list_;
    CComboBox mode_,encoding_,scope_;
    std::vector<simple_editor::services::MultiSearchMatch> matches_;
    std::future<simple_editor::services::MultiSearchSummary> job_;
    std::atomic_bool cancel_{false};
    std::atomic_uint64_t files_{0};
    std::mutex mutex_;
    std::condition_variable available_;
    std::deque<std::vector<simple_editor::services::MultiSearchMatch>> pending_;
    simple_editor::services::MultiSearchSummary summary_;
};

#pragma once
#include <afxcmn.h>
#include "../services/batch_replace.h"
#include <future>
class CBatchReplaceDialog : public CDialog {
public:
    explicit CBatchReplaceDialog(CWnd* owner);
    ~CBatchReplaceDialog() override;
    std::function<bool(const std::vector<std::wstring>&,bool,const std::vector<simple_editor::core::FileStamp>&)> lock_paths;
    friend class NativeUiTest;
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM,LPARAM) override;
    void OnOK() override;
    void OnCancel() override;
    afx_msg void OnTimer(UINT_PTR);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()
private:
    bool StartPlan(const std::filesystem::path&,const simple_editor::services::MultiSearchRequest&);
    bool StartApply(const std::vector<std::size_t>& selected);
    void Tick(); void Review(); void ReleaseLocks();
    struct Planning { simple_editor::services::BatchReplacePlan plan; std::wstring error; };
    std::future<Planning> planning_; std::future<simple_editor::services::BatchReplaceResult> applying_;
    simple_editor::services::BatchReplacePlan plan_; std::atomic_bool cancel_{false}; std::atomic_uint progress_{0};
    CComboBox syntax_,encoding_; std::vector<std::wstring> locked_; bool closing_ = false;
};

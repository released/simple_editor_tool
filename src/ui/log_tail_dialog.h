#pragma once
#include "code_editor.h"
#include "../core/log_tail.h"
#include <future>
class CLogTailDialog : public CDialog {
public:
    void Show(CWnd* owner,const std::filesystem::path& path,simple_editor::core::TextEncoding encoding);
    ~CLogTailDialog() override;
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
    void Tick(); void Stop(); CCodeEditor editor_; CComboBox encoding_; std::filesystem::path path_; simple_editor::core::FileStamp stamp_; std::future<std::pair<simple_editor::core::LogTailResult,std::wstring>> job_; std::atomic_bool cancel_{false}; ULONGLONG last_ = 0; bool paused_ = false;
};

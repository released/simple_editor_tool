#include <afxwin.h>
#include <shellapi.h>
#include "workspace_frame.h"
#include "resource.h"
#include "ui/code_editor.h"
#include "services/cli_console.h"

static BOOL CALLBACK FindMainWindow(HWND window, LPARAM result) {
    if (GetPropW(window, L"SimpleEditorTool.Native.MainWindow")) {
        *reinterpret_cast<HWND*>(result) = window;
        return FALSE;
    }
    return TRUE;
}

class CSimpleEditorApp : public CWinApp {
public:
    BOOL InitInstance() override {
        if (RunHeadlessCommandLine(__argc,__wargv,&cli_exit_)) return FALSE;
        CWinApp::InitInstance(); SetRegistryKey(L"SimpleEditorTool");
        mutex_ = CreateMutexW(nullptr, FALSE, L"Local\\SimpleEditorTool.Native.Singleton");
        const DWORD mutex_error = GetLastError();
        if (!mutex_) { AfxMessageBox(L"Could not create the application instance guard.", MB_ICONERROR); return FALSE; }
        if (mutex_error == ERROR_ALREADY_EXISTS) {
            HWND existing = nullptr;
            const auto deadline = GetTickCount64() + 5000;
            do {
                EnumWindows(FindMainWindow, reinterpret_cast<LPARAM>(&existing));
                if (existing) break;
                Sleep(25);
            } while (GetTickCount64() < deadline);
            if (existing && __argc > 1) {
                if (::GetPropW(existing, L"SimpleEditorTool.Native.WorkspaceVersion") != reinterpret_cast<HANDLE>(2)) {
                    AfxMessageBox(L"An older version is running. Save and close it before opening files with this version.", MB_ICONWARNING); return FALSE;
                }
                std::vector<wchar_t> paths;
                for (int i = 1; i < __argc; ++i) { const auto length = wcslen(__wargv[i]); paths.insert(paths.end(), __wargv[i], __wargv[i] + length + 1); }
                paths.push_back(L'\0');
                COPYDATASTRUCT data = {2, static_cast<DWORD>(paths.size() * sizeof(wchar_t)), paths.data()};
                DWORD_PTR accepted = 0;
                if (!SendMessageTimeoutW(existing, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&data), SMTO_ABORTIFHUNG | SMTO_BLOCK, 5000, &accepted) || !accepted) {
                    AfxMessageBox(L"The running instance could not accept these files. Retry after it responds.", MB_ICONWARNING); return FALSE;
                }
            }
            if (!existing && __argc > 1) AfxMessageBox(L"The running instance has not finished starting. Retry opening these files.", MB_ICONWARNING);
            if (existing) { ShowWindow(existing, SW_SHOWMAXIMIZED); SetForegroundWindow(existing); } return FALSE;
        }
        auto* frame = new CWorkspaceFrame(); m_pMainWnd = frame;
        for (int i = 1; i < __argc; ++i) frame->OpenPath(__wargv[i]);
        frame->ShowWindow(SW_SHOWMAXIMIZED); frame->UpdateWindow(); return TRUE;
    }
    int ExitInstance() override { CCodeEditor::ReleaseResources(); if (mutex_) CloseHandle(mutex_); const auto normal = CWinApp::ExitInstance(); return cli_exit_ >= 0 ? cli_exit_ : normal; }
private: HANDLE mutex_ = nullptr; int cli_exit_ = -1;
};

CSimpleEditorApp theApp;


#include <afxwin.h>
#include <shellapi.h>
#include <filesystem>
#include "workspace_frame.h"
#include "resource.h"
#include "ui/code_editor.h"
#include "ui/localization.h"
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
        std::vector<std::wstring> arguments;
        for (int i = 1; i < __argc; ++i) arguments.emplace_back(__wargv[i]);
        std::vector<simple_editor::core::OpenLocation> locations;
        if (!simple_editor::core::ParseOpenLocations(arguments,&locations)) {
            cli_exit_ = 2; UiMessageBox(L"Usage: SimpleEditorTool.exe --open <file> [--line <positive line number>]", MB_ICONERROR); return FALSE;
        }
        for (auto& location : locations) {
            std::error_code error; const auto absolute = std::filesystem::absolute(location.path,error);
            if (error) { cli_exit_ = 2; UiMessageBox(L"Could not resolve the requested file path.",MB_ICONERROR); return FALSE; }
            location.path = absolute.lexically_normal().wstring();
        }
        const bool positioned = !locations.empty() && locations.front().line != 0;
        mutex_ = CreateMutexW(nullptr, FALSE, L"Local\\SimpleEditorTool.Native.Singleton");
        const DWORD mutex_error = GetLastError();
        if (!mutex_) { UiMessageBox(L"Could not create the application instance guard.", MB_ICONERROR); return FALSE; }
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
                    UiMessageBox(L"An older version is running. Save and close it before opening files with this version.", MB_ICONWARNING); return FALSE;
                }
                if (positioned && ::GetPropW(existing, L"SimpleEditorTool.Native.OpenLocationVersion") != reinterpret_cast<HANDLE>(1)) {
                    cli_exit_ = 2; UiMessageBox(L"The running version cannot receive a line location. Save and close it, then retry with the updated version.", MB_ICONWARNING); return FALSE;
                }
                std::vector<wchar_t> paths;
                for (const auto& location : locations) {
                    paths.insert(paths.end(),location.path.begin(),location.path.end()); paths.push_back(L'\0');
                    if (positioned) { const auto line = std::to_wstring(location.line); paths.insert(paths.end(),line.begin(),line.end()); paths.push_back(L'\0'); }
                }
                paths.push_back(L'\0');
                if (paths.size() > 1024 * 1024 / sizeof(wchar_t)) { cli_exit_ = 2; UiMessageBox(L"Too many file paths to forward.",MB_ICONERROR); return FALSE; }
                COPYDATASTRUCT data = {positioned ? 3u : 2u, static_cast<DWORD>(paths.size() * sizeof(wchar_t)), paths.data()};
                DWORD_PTR accepted = 0;
                if (!SendMessageTimeoutW(existing, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&data), SMTO_ABORTIFHUNG | SMTO_BLOCK, 5000, &accepted) || !accepted) {
                    UiMessageBox(L"The running instance could not accept these files. Retry after it responds.", MB_ICONWARNING); return FALSE;
                }
            }
            if (!existing && __argc > 1) UiMessageBox(L"The running instance has not finished starting. Retry opening these files.", MB_ICONWARNING);
            if (existing) { ShowWindow(existing, SW_SHOWMAXIMIZED); SetForegroundWindow(existing); } return FALSE;
        }
        auto* frame = new CWorkspaceFrame(); m_pMainWnd = frame;
        for (const auto& location : locations) frame->OpenLocation(location);
        frame->ShowWindow(SW_SHOWMAXIMIZED); frame->UpdateWindow(); return TRUE;
    }
    int ExitInstance() override { simple_editor::ui::ShutdownLocalization(); CCodeEditor::ReleaseResources(); if (mutex_) CloseHandle(mutex_); const auto normal = CWinApp::ExitInstance(); return cli_exit_ >= 0 ? cli_exit_ : normal; }
private: HANDLE mutex_ = nullptr; int cli_exit_ = -1;
};

CSimpleEditorApp theApp;


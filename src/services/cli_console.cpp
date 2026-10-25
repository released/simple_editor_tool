#include "cli_console.h"
#include "../core/command_line.h"
#include <Windows.h>
namespace {
std::atomic_bool cancelled{false};
BOOL WINAPI CancelConsole(DWORD event) { if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT || event == CTRL_CLOSE_EVENT) { cancelled = true; return TRUE; } return FALSE; }
void Print(DWORD stream,const std::wstring& message) {
    HANDLE output = GetStdHandle(stream); DWORD mode = 0,written = 0;
    if (output == nullptr || output == INVALID_HANDLE_VALUE) return;
    const auto text = message + (message.empty() || message.back() == L'\n' ? L"" : L"\r\n");
    if (GetConsoleMode(output,&mode)) { WriteConsoleW(output,text.data(),static_cast<DWORD>(text.size()),&written,nullptr); return; }
    const int length = WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr); if (length <= 0) return;
    std::string bytes(length,0); WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),bytes.data(),length,nullptr,nullptr); WriteFile(output,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr);
}
}
bool RunHeadlessCommandLine(int argc,wchar_t** argv,int* exit_code) {
    if (argc < 2 || (std::wstring(argv[1]) != L"--compare" && std::wstring(argv[1]) != L"--help")) return false;
    const auto output = GetStdHandle(STD_OUTPUT_HANDLE); if (!output || output == INVALID_HANDLE_VALUE) AttachConsole(ATTACH_PARENT_PROCESS);
    cancelled = false; SetConsoleCtrlHandler(CancelConsole,TRUE); simple_editor::core::SaveControl control; control.cancelled = &cancelled; std::vector<std::wstring> arguments; for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
    const auto result = simple_editor::core::CompareCommandLine(arguments,control); SetConsoleCtrlHandler(CancelConsole,FALSE); Print(result.exit_code >= 2 ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE,result.message); *exit_code = result.exit_code; return true;
}

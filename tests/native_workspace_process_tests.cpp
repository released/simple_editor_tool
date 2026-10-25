#include <afxwin.h>
#include <afxcmn.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <climits>
#include <ShlObj.h>
#include <atlimage.h>
#include "../src/resource.h"
#include "../src/services/explorer_menu.h"

namespace {
std::vector<char> Bytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary); return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
std::wstring Quote(const std::filesystem::path& path) { return L"\"" + path.wstring() + L"\""; }
struct Process {
    PROCESS_INFORMATION info{};
    HWND frame = nullptr;
    ~Process() {
        if (!info.hProcess) return;
        if (WaitForSingleObject(info.hProcess, 0) == WAIT_TIMEOUT) {
            if (frame) PostMessageW(frame, WM_CLOSE, 0, 0);
            if (WaitForSingleObject(info.hProcess, 3000) == WAIT_TIMEOUT) {
                std::wcerr << L"Fixture cleanup terminated only test-owned PID " << info.dwProcessId << L'\n';
                TerminateProcess(info.hProcess, 9); WaitForSingleObject(info.hProcess, 3000);
            }
        }
        if (info.hThread) CloseHandle(info.hThread); CloseHandle(info.hProcess);
    }
    bool Start(const std::filesystem::path& executable, const std::vector<std::filesystem::path>& files) {
        std::wstring command = Quote(executable); for (const auto& file : files) command += L" " + Quote(file);
        STARTUPINFOW startup{}; startup.cb = sizeof(startup); startup.dwFlags = STARTF_USESHOWWINDOW; startup.wShowWindow = SW_HIDE;
        return CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr,
                              executable.parent_path().c_str(), &startup, &info) != FALSE;
    }
    bool Exited() const {
        DWORD code = 1; return info.hProcess && WaitForSingleObject(info.hProcess, 15000) == WAIT_OBJECT_0 && GetExitCodeProcess(info.hProcess, &code) && code == 0;
    }
    bool ShellOpen(const std::filesystem::path& file) {
        SHELLEXECUTEINFOW launch{sizeof(launch)}; launch.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC;
        launch.lpVerb = simple_editor::shell::kExplorerVerb; launch.lpFile = file.c_str(); launch.nShow = SW_HIDE;
        if (!ShellExecuteExW(&launch) || !launch.hProcess) return false;
        info.hProcess = launch.hProcess; info.dwProcessId = GetProcessId(launch.hProcess); return true;
    }
};
struct Frames { DWORD first, second; std::vector<HWND> windows; };
BOOL CALLBACK FindFrames(HWND window, LPARAM data) {
    auto* frames = reinterpret_cast<Frames*>(data); DWORD pid = 0; GetWindowThreadProcessId(window, &pid);
    if ((pid == frames->first || pid == frames->second) && GetPropW(window, L"SimpleEditorTool.Native.MainWindow")) frames->windows.push_back(window);
    return TRUE;
}

std::vector<HWND> OwnedFrames(DWORD first, DWORD second = 0) {
    Frames frames{first, second, {}}; EnumWindows(FindFrames, reinterpret_cast<LPARAM>(&frames)); return frames.windows;
}
bool WaitCount(HWND frame, int expected) {
    const auto deadline = GetTickCount64() + 15000;
    while (GetTickCount64() < deadline && IsWindow(frame)) {
        DWORD_PTR count = 0;
        if (SendMessageTimeoutW(GetDlgItem(frame, 600), TCM_GETITEMCOUNT, 0, 0, SMTO_ABORTIFHUNG, 1000, &count) && static_cast<int>(count) == expected) return true;
        Sleep(20);
    }
    return false;
}
}

namespace {
bool OwnedMenuCommand(HWND frame,DWORD process,UINT command) {
    std::wcout << L"Shell fixture command " << command << L", initial enabled=" << IsWindowEnabled(frame) << L'\n';
    PostMessageW(frame,WM_COMMAND,command,0); bool seen = false; const auto until = GetTickCount64() + 10000;
    struct Dialogs { DWORD process; bool* seen; } dialogs{process,&seen};
    while (GetTickCount64() < until) {
        EnumWindows([](HWND window,LPARAM parameter)->BOOL { const auto* data = reinterpret_cast<Dialogs*>(parameter); DWORD process = 0; GetWindowThreadProcessId(window,&process); wchar_t name[64]{}; GetClassNameW(window,name,64); if (process == data->process && wcscmp(name,L"#32770") == 0) { const auto button = GetDlgItem(window,IDOK) ? IDOK : GetDlgItem(window,IDCANCEL) ? IDCANCEL : 0; if (button) { *data->seen = true; PostMessageW(window,WM_COMMAND,button,0); } } return TRUE; },reinterpret_cast<LPARAM>(&dialogs));
        if (seen && IsWindowEnabled(frame)) return true; Sleep(20);
    }
    std::wcout << L"Shell fixture dialog seen=" << seen << L", final enabled=" << IsWindowEnabled(frame) << L'\n';
    EnumWindows([](HWND window,LPARAM parameter)->BOOL { DWORD process = 0; GetWindowThreadProcessId(window,&process); wchar_t name[64]{}; GetClassNameW(window,name,64); if (process == static_cast<DWORD>(parameter) && wcscmp(name,L"#32770") == 0) { RECT rect{}; GetWindowRect(window,&rect); if (rect.right > rect.left && rect.bottom > rect.top) { CImage image; image.Create(rect.right - rect.left,rect.bottom - rect.top,32); const auto dc = image.GetDC(); PrintWindow(window,dc,0); image.ReleaseDC(); const auto folder = std::filesystem::current_path() / L"artifacts/screenshots"; std::filesystem::create_directories(folder); image.Save((folder / L"explorer-dialog-failure.png").c_str()); } } return TRUE; },process);
    EnumWindows([](HWND window,LPARAM parameter)->BOOL { DWORD process = 0; GetWindowThreadProcessId(window,&process); if (process == static_cast<DWORD>(parameter)) { wchar_t name[128]{},title[1024]{}; GetClassNameW(window,name,128); GetWindowTextW(window,title,1024); std::wcout << L"Owned window: " << name << L" title=" << title << L'\n'; EnumChildWindows(window,[](HWND child,LPARAM)->BOOL { wchar_t text[1024]{}; GetWindowTextW(child,text,1024); if (*text) std::wcout << L"Owned control " << GetDlgCtrlID(child) << L": " << text << L'\n'; return TRUE; },0); } return TRUE; },process);
    return false;
}
bool NativeFileMenu(const std::filesystem::path& file,HWND owner) {
    PIDLIST_ABSOLUTE item = nullptr; PCUITEMID_CHILD child = nullptr; IShellFolder* folder = nullptr; IContextMenu* context = nullptr; bool found = false;
    HMENU menu = CreatePopupMenu(); const auto initialized = CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(SHParseDisplayName(file.c_str(),nullptr,&item,0,nullptr)) && SUCCEEDED(SHBindToParent(item,IID_PPV_ARGS(&folder),&child)) && SUCCEEDED(folder->GetUIObjectOf(owner,1,&child,IID_IContextMenu,nullptr,reinterpret_cast<void**>(&context))) && SUCCEEDED(context->QueryContextMenu(menu,0,1,32767,CMF_NORMAL))) {
        for (int n = 0; n < GetMenuItemCount(menu); ++n) { const auto id = GetMenuItemID(menu,n); wchar_t verb[256]{},label[256]{}; if (id && id != UINT_MAX && SUCCEEDED(context->GetCommandString(id - 1,GCS_VERBW,nullptr,reinterpret_cast<LPSTR>(verb),256)) && _wcsicmp(verb,simple_editor::shell::kExplorerVerb) == 0) { GetMenuStringW(menu,n,label,256,MF_BYPOSITION); found = wcsstr(label,L"Simple Editor Tool") != nullptr; break; } }
    }
    if (context) context->Release(); if (folder) folder->Release(); CoTaskMemFree(item); DestroyMenu(menu); if (SUCCEEDED(initialized)) CoUninitialize(); return found;
}
}

int RunExplorerProcessTests(const std::filesystem::path& root,const std::filesystem::path& executable) {
    using namespace simple_editor::shell;
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; return ok; };
    ExplorerMenuState state; std::wstring error;
    if (!ReadUserExplorerMenu(&state,&error) || state.present) { std::wcerr << L"NOT RUN: this isolated live-shell test requires the product verb to be absent; existing registration retained.\n"; return 1; }
    HANDLE existing = OpenMutexW(SYNCHRONIZE,FALSE,L"Local\\SimpleEditorTool.Native.Singleton"); if (existing) { CloseHandle(existing); std::wcerr << L"NOT RUN: a normal application owns the singleton.\n"; return 1; }
    const auto folder = root / L"\x4E2D\x6587 tool with spaces"; std::filesystem::create_directories(folder); const auto copy = folder / L"Editor renamed.exe";
    if (!check(CopyFileW(executable.c_str(),copy.c_str(),TRUE) && Bytes(copy) == Bytes(executable),L"isolate a byte-matched formal executable with a Unicode and spaced path")) return failures;
    struct Cleanup { std::wstring copy; ~Cleanup() { ExplorerMenuState state; std::wstring error; if (ReadUserExplorerMenu(&state,&error) && state.owned && state.executable == copy) SetUserExplorerMenu(L"",false,&error); } } cleanup{copy.wstring()};
    const auto first_file = root / L"old file.txt",second_file = root / L"\x4E2D\x6587 & 100% data.bin",third_file = root / L"third file.hex";
    for (const auto& file : {first_file,second_file,third_file}) { std::ofstream stream(file,std::ios::binary); stream << "unchanged source\r\n"; }
    const auto original = Bytes(first_file); Process primary;
    if (!check(primary.Start(copy,{first_file}),L"start the formal application with an existing file tab")) return failures;
    const auto until = GetTickCount64() + 15000; std::vector<HWND> frames;
    do { frames = OwnedFrames(primary.info.dwProcessId); if (!frames.empty()) break; Sleep(20); } while (GetTickCount64() < until);
    if (!check(frames.size() == 1 && WaitCount(frames[0],1),L"formal Explorer fixture owns exactly its expected workspace")) return failures; primary.frame = frames[0];
    if (!check(OwnedMenuCommand(primary.frame,primary.info.dwProcessId,ID_EXPLORER_ADD) && ReadUserExplorerMenu(&state,&error) && state.owned && state.executable == copy.wstring(),L"the actual Tools command installs only the current user's file context menu")) return failures;
    check(NativeFileMenu(second_file,primary.frame),L"Windows builds a native file context menu containing the registered Simple Editor Tool verb");
    { Process opened; check(opened.ShellOpen(second_file) && opened.Exited() && WaitCount(primary.frame,2),L"the actual shell verb opens Unicode, spaces, ampersand and percent filenames in a retained second tab"); }
    { Process opened; check(opened.ShellOpen(third_file) && opened.Exited() && WaitCount(primary.frame,3),L"another shell invocation appends a third file to the same workspace"); }
    { Process opened; check(opened.ShellOpen(second_file) && opened.Exited() && WaitCount(primary.frame,3),L"repeated shell opening activates the existing tab without replacing other files"); }
    check(OwnedFrames(primary.info.dwProcessId).size() == 1 && IsZoomed(primary.frame),L"shell opening retains a single maximized formal workspace");
    check(OwnedMenuCommand(primary.frame,primary.info.dwProcessId,ID_EXPLORER_REMOVE) && ReadUserExplorerMenu(&state,&error) && !state.present,L"the actual Tools removal command removes its own Explorer entry");
    PostMessageW(primary.frame,WM_CLOSE,0,0); check(primary.Exited(),L"the shell-opened formal application closes normally"); primary.frame = nullptr;
    check(Bytes(first_file) == original && Bytes(second_file) == original && Bytes(third_file) == original,L"Explorer activation leaves all source files untouched");
    return failures;
}

int RunWorkspaceProcessTests(const std::filesystem::path& root, const std::filesystem::path& executable) {
    int failures = 0;
    const auto check = [&](bool condition, const wchar_t* label) { if (!condition) { ++failures; std::wcerr << L"FAIL: " << label << L'\n'; } else std::wcout << L"PASS: " << label << L'\n'; return condition; };
    HANDLE existing = OpenMutexW(SYNCHRONIZE, FALSE, L"Local\\SimpleEditorTool.Native.Singleton");
    if (existing) { CloseHandle(existing); std::wcerr << L"NOT RUN: a normal application instance already owns the singleton.\n"; return 1; }
    const auto first_dir = root / L"formal-one", second_dir = root / L"formal-two";
    std::filesystem::create_directories(first_dir); std::filesystem::create_directories(second_dir);
    const auto first_exe = first_dir / L"SimpleEditorTool.exe", second_exe = second_dir / L"RenamedEditor.exe";
    if (!check(CopyFileW(executable.c_str(), first_exe.c_str(), TRUE) && CopyFileW(executable.c_str(), second_exe.c_str(), TRUE), L"isolate formal EXE copies without user settings")) return failures;
    const auto original = Bytes(executable);
    if (!check(!original.empty() && original == Bytes(first_exe) && original == Bytes(second_exe), L"formal and renamed EXE copies match every source byte")) return failures;
    const auto a = root / L"process-A.txt", b = root / L"process-B.bin", c = root / L"process-第三份.txt";
    for (const auto& file : {a,b,c}) { std::ofstream output(file, std::ios::binary); output << "process fixture\r\n"; }
    Process first, second;
    if (!check(first.Start(first_exe, {a,b}) && second.Start(second_exe, {c,a}), L"start concurrent formal and renamed instances with several file paths")) return failures;
    std::vector<HWND> frames; const auto deadline = GetTickCount64() + 15000;
    do { frames = OwnedFrames(first.info.dwProcessId, second.info.dwProcessId); if (!frames.empty()) break; Sleep(20); } while (GetTickCount64() < deadline);
    if (!check(frames.size() == 1, L"concurrent launches publish exactly one normal workspace")) return failures;
    DWORD owner = 0; GetWindowThreadProcessId(frames[0], &owner);
    Process& primary = owner == first.info.dwProcessId ? first : second;
    Process& secondary = owner == first.info.dwProcessId ? second : first;
    primary.frame = frames[0];
    if (!check(secondary.Exited() && WaitCount(primary.frame, 3), L"secondary forwards all paths and duplicate activation retains three file tabs")) return failures;
    check(IsZoomed(primary.frame) != FALSE,L"ready formal workspace starts maximized");
    check(OwnedFrames(first.info.dwProcessId, second.info.dwProcessId).size() == 1, L"renamed-copy forwarding creates no second top-level workspace");
    PostMessageW(primary.frame, WM_COMMAND, ID_FILE_CLOSE, 0);
    check(WaitCount(primary.frame, 2), L"formal Close tab removes only the explicitly selected file");
    PostMessageW(primary.frame, WM_CLOSE, 0, 0);
    if (!check(primary.Exited(), L"formal multi-session workspace closes normally with exit zero")) return failures;
    primary.frame = nullptr;
    Process relaunched;
    if (!check(relaunched.Start(second_exe, {b}), L"relaunch the renamed formal EXE after normal close")) return failures;
    const auto relaunch_deadline = GetTickCount64() + 15000;
    do { frames = OwnedFrames(relaunched.info.dwProcessId); if (!frames.empty()) break; Sleep(20); } while (GetTickCount64() < relaunch_deadline);
    if (!check(frames.size() == 1 && WaitCount(frames[0], 1), L"normal close releases the singleton for the next workspace")) return failures;
    relaunched.frame = frames[0]; PostMessageW(relaunched.frame, WM_CLOSE, 0, 0);
    check(relaunched.Exited(), L"relaunched formal workspace shuts down normally"); relaunched.frame = nullptr;
    return failures;
}

    static int RunExplorer(const std::filesystem::path& root) {
        using namespace simple_editor::shell;
        int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; return ok; };
        const auto registry_path = L"Software\\SimpleEditorTool\\Tests\\Explorer-" + root.filename().wstring();
        HKEY classes = nullptr; DWORD created = 0; if (!check(RegCreateKeyExW(HKEY_CURRENT_USER,registry_path.c_str(),0,nullptr,0,KEY_ALL_ACCESS,nullptr,&classes,&created) == ERROR_SUCCESS && created == REG_CREATED_NEW_KEY,L"create isolated per-user registry fixture")) { if (classes) RegCloseKey(classes); return failures; }
        struct Cleanup { HKEY key; std::wstring path; ~Cleanup() { RegCloseKey(key); RegDeleteTreeW(HKEY_CURRENT_USER,path.c_str()); } } cleanup{classes,registry_path};
        ExplorerMenuState state; std::wstring error;
        check(ReadExplorerMenu(classes,&state,&error) && !state.present,L"querying an absent registration performs no installation");
        check(SetExplorerMenu(classes,L"",false,&error),L"removing an absent owned menu is idempotent");
        check(!SetExplorerMenu(classes,L"relative.exe",true,&error) && ReadExplorerMenu(classes,&state,&error) && !state.present,L"invalid relative executable cannot leave a registry entry");
        const auto folder = root / L"\x4E2D\x6587 & spaced path"; std::filesystem::create_directories(folder);
        const auto executable = folder / L"renamed editor.exe"; CopyFileW(CurrentExecutable().c_str(),executable.c_str(),TRUE);
        check(SetExplorerMenu(classes,executable.wstring(),true,&error) && ReadExplorerMenu(classes,&state,&error) && state.present && state.owned && state.executable == executable.wstring(),L"install an owned registration with a Unicode and spaced executable path");
        const auto value = [&](const wchar_t* child,const wchar_t* name) { wchar_t text[32768]{}; DWORD size = sizeof(text); return RegGetValueW(classes,child,name,RRF_RT_REG_SZ,nullptr,text,&size) == ERROR_SUCCESS ? std::wstring(text) : L""; };
        check(value((std::wstring(kExplorerKey) + L"\\command").c_str(),nullptr) == L"\"" + executable.wstring() + L"\" \"%1\"",L"registry command quotes both the executable and selected filename without a shell wrapper");
        check(value(kExplorerKey,L"MUIVerb") == L"\x4EE5 Simple Editor Tool \x958B\x555F" && value(kExplorerKey,L"Icon") == L"\"" + executable.wstring() + L"\",0" && value(kExplorerKey,L"MultiSelectModel") == L"Player",L"menu carries its localized name, executable icon and explicit multi-selection model");
        check(SetExplorerMenu(classes,CurrentExecutable(),true,&error) && ReadExplorerMenu(classes,&state,&error) && state.executable == CurrentExecutable(),L"adding again refreshes the registered portable executable location");
        check(!SetExplorerMenu(classes,(root / L"missing.exe").wstring(),true,&error) && value((std::wstring(kExplorerKey) + L"\\command").c_str(),nullptr) == L"\"" + CurrentExecutable() + L"\" \"%1\"",L"an invalid update retains the previous working command");
        HKEY sibling = nullptr; RegCreateKeyExW(classes,L"*\\shell\\unrelated",0,nullptr,0,KEY_ALL_ACCESS,nullptr,&sibling,nullptr); RegCloseKey(sibling); sibling = nullptr;
        check(SetExplorerMenu(classes,L"",false,&error) && ReadExplorerMenu(classes,&state,&error) && !state.present && RegOpenKeyExW(classes,L"*\\shell\\unrelated",0,KEY_READ,&sibling) == ERROR_SUCCESS,L"removal deletes only the owned verb and preserves sibling applications"); if (sibling) RegCloseKey(sibling);
        HKEY foreign = nullptr; RegCreateKeyExW(classes,kExplorerKey,0,nullptr,0,KEY_ALL_ACCESS,nullptr,&foreign,nullptr); const wchar_t data[] = L"foreign entry"; RegSetValueExW(foreign,nullptr,0,REG_SZ,reinterpret_cast<const BYTE*>(data),sizeof(data));
        check(!SetExplorerMenu(classes,executable.wstring(),true,&error) && !SetExplorerMenu(classes,L"",false,&error) && value(kExplorerKey,nullptr) == data,L"install and removal refuse an unowned entry with the same name"); RegCloseKey(foreign);
        auto* workspace = new CWorkspaceFrame(false); AfxGetApp()->m_pMainWnd = workspace;
        auto commands = CollectPaletteCommands(workspace->GetMenu());
        check(std::any_of(commands.begin(),commands.end(),[](const auto& item) { return item.id == ID_EXPLORER_ADD; }) && std::any_of(commands.begin(),commands.end(),[](const auto& item) { return item.id == ID_EXPLORER_REMOVE; }),L"Explorer controls are available from Tools before any file is opened");
        Write(root / L"menu.txt",{'x'}); workspace->OpenPath((root / L"menu.txt").wstring()); commands = CollectPaletteCommands(workspace->GetMenu());
        struct Availability : CCmdUI { bool enabled = false; void Enable(BOOL value) override { enabled = value != FALSE; } void SetCheck(int) override {} } update; update.m_nID = ID_EXPLORER_ADD;
        check(std::any_of(commands.begin(),commands.end(),[](const auto& item) { return item.id == ID_EXPLORER_ADD; }) && workspace->OnCmdMsg(ID_EXPLORER_ADD,CN_UPDATE_COMMAND_UI,&update,nullptr) && update.enabled,L"Explorer menu and native workspace command routing remain available with an active file");
        workspace->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr; return failures;
    }

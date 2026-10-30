    inline static unsigned notice_count_ = 0;
    inline static bool notice_guarded_ = true, notice_capture_ok_ = false;
    inline static std::filesystem::path notice_capture_;
    inline static CMainFrame* notice_frame_ = nullptr;
    static void CALLBACK CompareNoticeTimer(HWND,UINT,UINT_PTR,DWORD) {
        const auto dialog = TestDialog(); if (!dialog) return;
        wchar_t title[256]{}; ::GetWindowTextW(dialog,title,256);
        if (!AfxGetApp()->m_pszAppName || wcscmp(title,AfxGetApp()->m_pszAppName) != 0) return;
        CMainFrame* owner = notice_frame_;
        const auto matches = [](CMainFrame* frame) { return frame && frame->path_dialog_ && (frame->displayed_mode_ < 2 || frame->advanced_.modal_); };
        if (!owner) {
            if (auto* workspace = dynamic_cast<CWorkspaceFrame*>(AfxGetApp()->m_pMainWnd)) { for (auto* frame : workspace->sessions_) if (matches(frame)) { owner = frame; break; } }
            else owner = dynamic_cast<CMainFrame*>(AfxGetApp()->m_pMainWnd);
        }
        if (!matches(owner)) return;
        ++notice_count_; notice_guarded_ = notice_guarded_ && owner->IsBusy();
        if (!notice_capture_.empty()) { notice_capture_ok_ = ExpansionCapture(*CWnd::FromHandle(dialog),notice_capture_); notice_capture_.clear(); }
        notice_guarded_ = (::EndDialog(dialog,IDOK) != FALSE) && notice_guarded_;
    }
    static int RunCompareNotice(const std::filesystem::path& root,const std::filesystem::path& image = {}) {
        using namespace simple_editor::core;
        int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
        std::wcout << std::unitbuf;
        notice_count_ = 0; notice_guarded_ = true; notice_capture_ok_ = false; notice_capture_.clear();
        const auto write = [&](const auto& path,const char* text) { const std::string bytes(text); Write(path,{bytes.begin(),bytes.end()}); };
        auto* workspace = new CWorkspaceFrame(false); AfxGetApp()->m_pMainWnd = workspace; workspace->preferences_path_ = root / L"settings.ini";
        workspace->SetWindowPos(nullptr,0,0,1280,900,SWP_NOZORDER | SWP_NOACTIVATE); workspace->ShowWindow(SW_SHOWNOACTIVATE);
        const auto timer = ::SetTimer(nullptr,0,30,CompareNoticeTimer);
        const auto left = root / L"left.txt",right = root / L"right.txt";
        write(left,"Alpha \n"); write(right,"Alpha \n");
        workspace->NewComparisonInputs(left.wstring(),right.wstring(),L"",2,true); auto* frame = workspace->sessions_.back(); notice_frame_ = frame; auto& view = frame->advanced_;
        check(Wait(frame) && notice_count_ == 1 && !frame->path_dialog_ && !view.modal_,L"equal text displays and dismisses one informational popup with modal guards restored");
        view.Tick(); frame->OnTimer(1); check(notice_count_ == 1,L"idle ticks cannot repeat an already consumed equality notice");
        frame->OnCompareRun(); check(Wait(frame) && notice_count_ == 2,L"a second explicit Compare displays its own completion notice");
        write(right,"different\n"); view.conflicts_only_.SetCheck(BST_CHECKED); frame->OnCompareRun();
        check(Wait(frame) && view.visible_.empty() && notice_count_ == 2,L"an empty filtered result cannot claim different inputs are equal");
        view.conflicts_only_.SetCheck(BST_UNCHECKED); write(right,"alpha\r\n"); view.ignore_case_.SetCheck(BST_CHECKED); view.ignore_space_.SetCheck(BST_CHECKED); view.ignore_eol_.SetCheck(BST_CHECKED);
        frame->OnCompareRun(); check(Wait(frame) && notice_count_ == 3 && !view.model_->blocks.empty(),L"equality under ignore settings uses meaningful blocks rather than raw row count");
        frame->OnCompareRun(); frame->OnCompareCancel(); check(Wait(frame) && notice_count_ == 3,L"cancelled equal-text comparison does not display success");
        Write(right,{0xff,0xff,0xff}); view.encoding_right_.SetCurSel(static_cast<int>(TextEncoding::Utf8)); frame->OnCompareRun();
        check(Wait(frame) && notice_count_ == 3,L"failed text decoding cannot display an equality notice");
        const auto binary_left = root / L"left.bin",binary_right = root / L"right.bin";
        Write(binary_left,{0,42,255}); Write(binary_right,{0,42,255}); auto* binary = new CMainFrame(false); binary->OpenPath(binary_left.wstring()); notice_frame_ = binary;
        binary->StartCompare(binary_right.wstring(),false); check(Wait(binary) && notice_count_ == 4,L"equal-address binary comparison displays no-differences popup");
        binary->StartCompare(binary_right.wstring(),true); check(Wait(binary) && notice_count_ == 5,L"insertion-aware binary comparison displays no-differences popup");
        Write(binary_right,{0,43,255}); binary->StartCompare(binary_right.wstring(),false); check(Wait(binary) && notice_count_ == 5 && !binary->pending_differences_.empty(),L"different binary contents do not display an equality notice");
        Write(binary_right,{0,42,255}); binary->StartCompare(binary_right.wstring(),false); binary->OnCompareCancel(); check(Wait(binary) && notice_count_ == 5,L"cancelled binary comparison does not display success");
        binary->StartCompare(binary_right.wstring(),false); ++binary->revision_; check(Wait(binary) && notice_count_ == 5,L"stale binary results cannot display success");
        binary->DestroyWindow();
        workspace->NewComparisonInputs(L"",L"",L"",8,false); auto* pasted = workspace->sessions_.back(); notice_frame_ = pasted; auto& inputs = pasted->advanced_;
        inputs.input_left_.SetWindowTextW(L"pasted\r\n"); inputs.input_right_.SetWindowTextW(L"pasted\r\n"); pasted->OnCompareRun();
        check(Wait(pasted) && notice_count_ == 6,L"equal pasted inputs display the same completion notice");
        inputs.input_right_.SetWindowTextW(L"changed"); pasted->OnCompareRun(); check(Wait(pasted) && notice_count_ == 6,L"different pasted inputs do not display an equality notice");
        inputs.input_left_.SetWindowTextW(L""); inputs.input_right_.SetWindowTextW(L""); pasted->OnCompareRun(); check(Wait(pasted) && notice_count_ == 7,L"two empty inputs are reported as equal");
        const auto a = root / L"a.json",b = root / L"b.json"; write(a,"{\"a\":1}"); write(b,"{\"a\":1}");
        workspace->NewComparisonInputs(a.wstring(),b.wstring(),L"",7,true); auto* json = workspace->sessions_.back(); notice_frame_ = json;
        check(Wait(json) && notice_count_ == 8,L"equal structured inputs display a completion notice");
        write(b,"{\"a\":2}"); json->OnCompareRun(); check(Wait(json) && notice_count_ == 8,L"structured differences do not display an equality notice");
        json->advanced_.job_ = std::async(std::launch::async,[]() { CAdvancedCompareView::Result result; result.kind = CAdvancedCompareView::JobKind::Structured; result.structured = std::make_shared<StructuredComparison>(); result.structured->truncated = true; return result; });
        check(Wait(json) && notice_count_ == 8,L"truncated structured results cannot claim equality");
        const auto folder_left = root / L"folder-left",folder_right = root / L"folder-right";
        std::filesystem::create_directories(folder_left); std::filesystem::create_directories(folder_right); write(folder_left / L"same.txt","same"); write(folder_right / L"same.txt","same");
        workspace->NewComparisonInputs(folder_left.wstring(),folder_right.wstring(),L"",4,true); auto* folders = workspace->sessions_.back(); notice_frame_ = folders;
        check(Wait(folders) && notice_count_ == 9,L"a complete equal folder comparison displays a notice");
        folders->advanced_.StartFolders(folder_left.wstring(),folder_right.wstring(),false);
        check(Wait(folders) && notice_count_ == 9,L"automatic folder rescan can suppress completion popups");
        write(folder_right / L"same.txt","different"); folders->OnCompareRun(); check(Wait(folders) && notice_count_ == 9,L"folder differences do not display an equality notice");
        folders->advanced_.StartFolders(folder_left.wstring(),(root / L"missing-folder").wstring()); check(Wait(folders) && notice_count_ == 9,L"failed folder scan does not display success");
        folders->advanced_.job_ = std::async(std::launch::async,[]() { CAdvancedCompareView::Result result; result.kind = CAdvancedCompareView::JobKind::Scan; result.scan = std::make_shared<FolderScan>(); result.scan->partial_refresh = true; return result; });
        check(Wait(folders) && notice_count_ == 9,L"partial folder results cannot claim complete equality");
        check(folders->advanced_.visible_.empty() && folders->advanced_.rows_.GetItemCount() == 0,L"replacing a populated folder scan with an empty result retires every old virtual row");
        const auto empty_left = root / L"empty-left",empty_right = root / L"empty-right";
        std::filesystem::create_directories(empty_left); std::filesystem::create_directories(empty_right);
        folders->advanced_.StartFolders(folder_left.wstring(),folder_right.wstring(),false); check(Wait(folders) && folders->advanced_.rows_.GetItemCount() > 0,L"full folder scan can repopulate the emptied virtual list");
        folders->advanced_.StartFolders(empty_left.wstring(),empty_right.wstring(),false); check(Wait(folders) && folders->advanced_.visible_.empty() && folders->advanced_.rows_.GetItemCount() == 0,L"a real scan from populated to empty folders keeps native painting in range");
        folders->advanced_.visible_ = {0};
        check(folders->advanced_.Cell(0,0).empty(),L"a late virtual text callback rejects a retired folder row");
        NMLVCUSTOMDRAW stale_draw{}; stale_draw.nmcd.dwDrawStage = CDDS_ITEMPREPAINT; stale_draw.nmcd.dwItemSpec = 0; LRESULT draw_result = 0;
        folders->advanced_.OnDraw(&stale_draw.nmcd.hdr,&draw_result); check(draw_result == CDRF_DODEFAULT,L"a late custom-draw callback rejects a retired folder row");
        folders->advanced_.visible_.clear();
        folders->advanced_.job_ = std::async(std::launch::async,[]() { CAdvancedCompareView::Result result; result.kind = CAdvancedCompareView::JobKind::Scan; result.scan = std::make_shared<FolderScan>(); FolderRow row; row.state = FolderState::Error; row.error = L"Synthetic unreadable input"; result.scan->rows.push_back(row); return result; });
        check(Wait(folders) && notice_count_ == 9,L"folder error rows cannot claim equality");
        write(left,"three\n"); write(right,"three\n"); const auto base = root / L"base.txt"; write(base,"three\n");
        workspace->NewComparisonInputs(left.wstring(),right.wstring(),base.wstring(),3,true); auto* three = workspace->sessions_.back(); notice_frame_ = three;
        check(Wait(three) && notice_count_ == 10,L"unchanged three-way inputs display a notice after complete comparison");
        const auto copy_left = root / L"copy-left.txt",copy_right = root / L"copy-right.txt"; write(copy_left,"left\n"); write(copy_right,"right\n");
        workspace->NewComparisonInputs(copy_left.wstring(),copy_right.wstring(),L"",2,true); auto* copy = workspace->sessions_.back(); notice_frame_ = copy; check(Wait(copy),L"directional-copy fixture completes");
        copy->advanced_.CopySide(true,true); check(Wait(copy) && notice_count_ == 10 && copy->advanced_.source_left_.Utf8() == copy->advanced_.source_right_.Utf8(),L"copy-to-equal automatic recomparison stays quiet and preserves draft workflow");
        std::wstring error; check(simple_editor::ui::InitializeLocalization(std::filesystem::current_path() / L"src" / L"res",L"tw",&error),L"load Traditional Chinese catalog for the actual information dialog");
        write(left,"localized\n"); write(right,"localized\n"); workspace->NewComparisonInputs(left.wstring(),right.wstring(),L"",2,true); auto* localized = workspace->sessions_.back(); notice_frame_ = localized; notice_capture_ = image;
        check(Wait(localized) && notice_count_ == 11 && simple_editor::ui::UiLanguage() == L"tw",L"no-differences popup is displayed with Traditional Chinese selected");
        if (!image.empty()) check(notice_capture_ok_,L"capture the actual localized no-differences information dialog");
        check(notice_guarded_,L"every notice keeps its session busy throughout the modal message loop");
        ::KillTimer(nullptr,timer); notice_frame_ = nullptr; notice_capture_.clear();
        for (auto* session : workspace->sessions_) { session->text_dirty_ = false; session->text_.SetModify(FALSE); session->document_.Close(); session->advanced_.dirty_ = false; }
        workspace->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr; simple_editor::ui::ShutdownLocalization(); return failures;
    }

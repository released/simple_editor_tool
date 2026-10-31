    static inline HWND reliability_owner_ = nullptr;
    static inline int reliability_answer_ = IDOK;
    static void CALLBACK ReliabilityDialogTimer(HWND,UINT,UINT_PTR,DWORD) {
        if (!::IsWindow(reliability_owner_)) return;
        const auto dialog = ::GetLastActivePopup(reliability_owner_); wchar_t name[40]{};
        wchar_t title[128]{}; ::GetWindowTextW(dialog,title,128);
        if (dialog != reliability_owner_ && ::GetWindowThreadProcessId(dialog,nullptr) == GetCurrentThreadId() &&
            ::GetClassNameW(dialog,name,40) && wcscmp(name,L"#32770") == 0 && AfxGetApp()->m_pszAppName && wcscmp(title,AfxGetApp()->m_pszAppName) == 0) ::EndDialog(dialog,reliability_answer_);
    }
    static CMainFrame* MakeSession(CWorkspaceFrame& workspace,bool activate) { return workspace.CWorkspaceFrame::CreateSession(activate); }
    static int RunReliability(const std::filesystem::path& root,const std::filesystem::path& screenshot = {}) {
        using namespace simple_editor::core;
        int failures = 0; const auto check = [&](bool ok,const wchar_t* name) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << name << L'\n'; if (!ok) ++failures; };
        std::wstring locale_error; check(simple_editor::ui::InitializeLocalization(std::filesystem::current_path() / L"src" / L"res",L"tw",&locale_error),L"reliability UI uses isolated source catalogs in Traditional Chinese");
        struct Workspace : CWorkspaceFrame {
            bool refuse = false;
            Workspace() : CWorkspaceFrame(false) {}
            CMainFrame* CreateSession(bool activate) override { return refuse ? nullptr : NativeUiTest::MakeSession(*this,activate); }
        };
        auto* workspace = new Workspace; AfxGetApp()->m_pMainWnd = workspace; reliability_owner_ = workspace->GetSafeHwnd();
        const auto timer = ::SetTimer(nullptr,0,20,ReliabilityDialogTimer);
        workspace->SetWindowPos(nullptr,0,0,1280,900,SWP_NOZORDER | SWP_NOACTIVATE); workspace->ShowWindow(SW_SHOWNOACTIVATE);
        const auto a = root / L"a.txt",b = root / L"b.txt",ini = root / L"preferences.ini";
        const unsigned line_count = 20000;
        std::string body; for (unsigned i = 0; i < line_count; ++i) body += "line " + std::to_string(i) + " value\n";
        Write(a,{body.begin(),body.end()}); auto right = body; right.replace(right.find("value"),5,"other"); Write(b,{right.begin(),right.end()});
        check(workspace->OpenPath(a.wstring()),L"reliability fixture opens a normal source document"); auto* editor = workspace->sessions_.front();
        editor->text_.SetSel(0,0); editor->text_.ReplaceSel(L"draft ",TRUE); const auto draft = editor->text_.Utf8(); const auto generation = editor->text_.Generation();
        workspace->refuse = true;
        check(!workspace->NewComparisonInputs(a.wstring(),b.wstring(),L"",2),L"comparison creation explicitly reports factory failure");
        WorkspaceEntry entry; entry.comparison = true; entry.left = a.wstring(); entry.right = b.wstring(); entry.mode = 2; entry.page = 1; entry.encoding = static_cast<int>(TextEncoding::Big5);
        workspace->RestoreWorkspace({entry}); workspace->ShowDiskComparison(L"draft",L"disk",a.wstring(),{},{});
        check(workspace->sessions_.size() == 1 && editor->text_.Utf8() == draft && editor->text_.Generation() == generation && editor->text_.CanUndo() && editor->requested_encoding_ == TextEncoding::Auto,L"failed workspace/disk comparison creation preserves the existing draft, codec and native Undo");
        workspace->refuse = false;
        auto* comparison = workspace->NewComparisonInputs(a.wstring(),b.wstring(),L"",2,true);
        const auto comparison_window = comparison ? comparison->GetSafeHwnd() : nullptr;
        const bool comparison_alive = comparison && Wait(comparison) && ::IsWindow(comparison_window) && CWnd::FromHandlePermanent(comparison_window) == comparison;
        auto* actual_view = comparison_alive ? dynamic_cast<CAdvancedCompareView*>(CWnd::FromHandlePermanent(::GetDlgItem(comparison_window,526))) : nullptr;
        check(comparison_alive && actual_view == &comparison->advanced_ && actual_view->model_,L"captured unsaved text compares through the background preparation path with a live owned view");
        if (!comparison_alive || !actual_view || actual_view != &comparison->advanced_ || !actual_view->model_) { workspace->DestroyWindow(); ::KillTimer(nullptr,timer); return failures + 1; }
        auto& view = comparison->advanced_;
        const auto output = root / L"locked-output.txt";
        view.locked_paths_ = {output.wstring()}; view.locks_held_ = workspace->LockPaths(view.locked_paths_,true,&view); view.job_kind_ = CAdvancedCompareView::JobKind::Save; view.SetBusy(true);
        const bool launched = view.LaunchJob([]() -> std::future<CAdvancedCompareView::Result> { throw std::bad_alloc(); });
        check(!launched && !view.Busy() && !workspace->PathLocked(output.wstring()) && view.save_.IsWindowEnabled(),L"failed worker launch releases path locks and restores controls");
        view.RenderCheckpoint(); const auto choice = view.model_->blocks.front().choice; view.model_->blocks.front().choice = MergeChoice::Right; view.job_kind_ = CAdvancedCompareView::JobKind::Render; view.SetBusy(true);
        view.LaunchJob([]() -> std::future<CAdvancedCompareView::Result> { throw std::bad_alloc(); });
        check(view.render_before_.empty() && view.model_->blocks.front().choice == choice,L"failed render launch restores the prior merge choices");
        view.source_left_.SetFocus(); view.source_left_.GoToLine(120); view.source_left_.Call(SCI_SETSEL,view.source_left_.Call(SCI_GETCURRENTPOS),view.source_left_.Call(SCI_GETCURRENTPOS) + 4); view.source_left_.Call(SCI_SETFIRSTVISIBLELINE,100);
        const auto anchor = view.source_left_.Call(SCI_GETANCHOR),caret = view.source_left_.Call(SCI_GETCURRENTPOS); const auto source_generation = view.source_left_.Generation();
        const auto top_line = view.source_left_.Call(SCI_DOCLINEFROMVISIBLE,view.source_left_.Call(SCI_GETFIRSTVISIBLELINE)); const auto source = view.source_left_.Utf8();
        view.similar_.SetCheck(!view.similar_.GetCheck()); view.OnSimilarChanged(); check(Wait(comparison),L"presentation-only alignment job completes");
        check(view.source_left_.Call(SCI_GETANCHOR) == anchor && view.source_left_.Call(SCI_GETCURRENTPOS) == caret && view.source_left_.Generation() == source_generation && view.source_left_.Utf8() == source && view.source_left_.Call(SCI_DOCLINEFROMVISIBLE,view.source_left_.Call(SCI_GETFIRSTVISIBLELINE)) == top_line,L"alignment retains source selection, original top line, content and generation");
        const auto clock = []() { return std::chrono::steady_clock::now(); }; volatile std::size_t checksum = 0;
        const auto lookup_line = static_cast<int>(line_count - 10);
        const auto linear_start = clock();
        for (int repeat = 0; repeat < 500; ++repeat) for (std::size_t row = 0; row < view.visible_.size(); ++row) if (view.text_rows_[view.visible_[row]].left == lookup_line) { checksum += row; break; }
        const auto linear_end = clock(); for (int repeat = 0; repeat < 500; ++repeat) checksum += view.source_rows_[0][lookup_line]; const auto indexed_end = clock();
        std::wcout << L"METRIC source_lookup_rows=" << view.visible_.size() << L" reference_linear_us=" << std::chrono::duration<double,std::micro>(linear_end-linear_start).count() << L" indexed_us=" << std::chrono::duration<double,std::micro>(indexed_end-linear_end).count() << L" checksum=" << checksum << L'\n';
        check(view.source_rows_[0][lookup_line] < view.visible_.size() && view.text_rows_[view.visible_[view.source_rows_[0][lookup_line]]].left == lookup_line,L"source line index maps directly to the original visible comparison row");
        view.output_.ReplaceSel(L"retained merge draft",TRUE); const auto merge = view.RawOutputText(); const auto old_model = view.model_;
        editor->text_.SetSel(0,0); editor->text_.ReplaceSel(L"new ",TRUE);
        check(view.sources_changed_ && (view.source_warning_.GetStyle() & WS_VISIBLE) && view.model_ == old_model && view.RawOutputText() == merge && view.output_.CanUndo(),L"source edits display a stale-snapshot banner while preserving sources and merge draft Undo");
        CString banner; view.source_warning_.GetWindowTextW(banner); check(banner.Find(L"來源文件已變更") >= 0,L"source-change banner is localized in Traditional Chinese");
        simple_editor::ui::SetUiLanguage(L"en"); view.source_warning_.GetWindowTextW(banner); check(banner.Find(L"Source documents have changed") == 0 && view.RawOutputText() == merge,L"banner language switches without modifying the retained merge buffer");
        simple_editor::ui::SetUiLanguage(L"tw"); comparison->Layout();
        if (!screenshot.empty()) check(ExpansionCapture(*workspace,screenshot),L"capture the native localized stale-source banner and retained comparison panes");
        view.dirty_ = false; comparison->OnCompareRun(); view.Cancel();
        check(Wait(comparison) && view.sources_changed_ && view.model_ == old_model && view.RawOutputText() == merge && !view.source_stamps_.empty(),L"cancelled refresh retains the stale banner, disk probes and previous merge output");
        editor->text_.SetSel(0,0); editor->text_.ReplaceSel(L"late ",TRUE); view.cancelled_ = false;
        auto late = std::make_shared<std::promise<CAdvancedCompareView::Result>>(); view.job_kind_ = CAdvancedCompareView::JobKind::Text; view.SetBusy(true); view.job_ = late->get_future();
        CAdvancedCompareView::Result result; result.kind = CAdvancedCompareView::JobKind::Text; result.model = std::make_shared<TextMergeModel>(); std::wstring late_error; result.left_snapshot = comparison->document_.Snapshot(&late_error); late->set_value(std::move(result)); view.Tick();
        check(view.model_ == old_model && view.RawOutputText() == merge,L"late comparison results cannot replace a snapshot after source edits"); view.dirty_ = false;
        view.SetSourcesChanged(false); view.source_probe_at_ = 0; Write(b,{'e','x','t','e','r','n','a','l'});
        const auto probe_end = GetTickCount64() + 5000; while (!view.sources_changed_ && GetTickCount64() < probe_end) { view.Tick(); Sleep(1); }
        check(view.sources_changed_ && view.RawOutputText() == merge,L"background disk freshness checks detect a changed unopened right input without discarding the merge draft");
        std::wstring opened_error; auto unopened = workspace->CaptureTextInput(b.wstring(),&opened_error); workspace->OpenPath(b.wstring());
        check(unopened && !workspace->TextInputCurrent(*unopened,b.wstring(),&opened_error),L"opening a previously unopened source during preparation requires a fresh comparison");
        AppPreferences preference; std::wstring error; check(SavePreferences(ini,preference,&error),L"isolated preferences fixture is saved"); workspace->preferences_path_ = ini;
        const auto handle = CreateFileW(ini.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        workspace->preferences_.font_tenths = 130; workspace->MarkPreferencesDirty(); workspace->StartPreferenceWrite();
        const auto wait_preferences = [&]() { const auto end = GetTickCount64() + 10000; while (workspace->preferences_io_.valid() && GetTickCount64() < end) { workspace->TickPreferences(); Sleep(1); } return !workspace->preferences_io_.valid(); };
        wait_preferences(); check(workspace->preferences_dirty_ && workspace->preferences_failed_ && !workspace->preferences_close_pending_,L"failed settings write retains pending state and cancels automatic exit");
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
        std::promise<std::wstring> failed_write; workspace->preferences_io_ = failed_write.get_future(); workspace->preferences_close_pending_ = true;
        failed_write.set_value(L"Synthetic preference write failure"); workspace->TickPreferences();
        check(::IsWindow(workspace->GetSafeHwnd()) && workspace->preferences_dirty_ && workspace->preferences_failed_ && !workspace->preferences_close_pending_,L"asynchronous settings failure cancels the pending exit without losing the unsaved version");
        for (int i = 0; i < 20; ++i) workspace->TickPreferences(); check(!workspace->preferences_io_.valid(),L"failed settings do not enter an automatic retry loop");
        workspace->SendMessage(WM_COMMAND,ID_WORKSPACE_RETRY_PREFERENCES); AppPreferences loaded;
        check(wait_preferences() && LoadPreferences(ini,&loaded,&error) && loaded.font_tenths == 130 && !workspace->preferences_dirty_,L"explicit settings retry persists the retained preference version");
        std::promise<std::wstring> pending; workspace->MarkPreferencesDirty(); workspace->preferences_write_revision_ = workspace->preferences_revision_; workspace->preferences_io_ = pending.get_future();
        workspace->preferences_.font_tenths = 140; workspace->MarkPreferencesDirty(); pending.set_value(L""); workspace->TickPreferences();
        check(wait_preferences() && LoadPreferences(ini,&loaded,&error) && loaded.font_tenths == 140 && !workspace->preferences_dirty_,L"changes arriving during a settings write persist as the newest version");
        const auto large_path = root / L"large.txt"; std::string large; large.reserve(8 * 1024 * 1024);
        for (int i = 0; i < 65536; ++i) large += std::to_string(i) + " English 中文 " + std::string(100,'x') + (i % 3 ? "\n" : "\r\n");
        Write(large_path,{large.begin(),large.end()}); workspace->OpenPath(large_path.wstring()); auto* large_editor = workspace->sessions_.back(); large_editor->text_.SetSel(0,0); large_editor->text_.ReplaceSel(L"edited ",TRUE);
        const auto sync_start = clock(); auto baseline = workspace->SnapshotForPath(large_path.wstring(),&error); const auto sync_end = clock();
        const auto capture_start = clock(); auto captured = workspace->CaptureTextInput(large_path.wstring(),&error); const auto capture_end = clock(); std::atomic_bool cancel{false};
        auto preparation = std::async(std::launch::async,[captured,&cancel,&error] { return captured && captured->Materialize(cancel,&error); }); std::size_t pumps = 0;
        while (preparation.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) { MSG message{}; while (::PeekMessage(&message,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&message); DispatchMessage(&message); } ++pumps; Sleep(1); }
        const bool prepared = preparation.get(); const auto finished = clock(); std::vector<std::uint8_t> before,after;
        check(prepared && baseline && baseline->Read(0,static_cast<std::size_t>(baseline->Size()),&before) && captured->document->Read(0,static_cast<std::size_t>(captured->document->Size()),&after) && before == after && pumps > 0,L"background mixed-EOL snapshot matches the synchronous reference while the UI pumps events");
        std::wcout << L"METRIC snapshot_bytes=" << large.size() << L" reference_sync_ms=" << std::chrono::duration<double,std::milli>(sync_end-sync_start).count() << L" capture_ui_ms=" << std::chrono::duration<double,std::milli>(capture_end-capture_start).count() << L" preparation_wall_ms=" << std::chrono::duration<double,std::milli>(finished-capture_end).count() << L" ui_pumps=" << pumps << L'\n';
        auto stale_capture = workspace->CaptureTextInput(large_path.wstring(),&error,true); large_editor->text_.ReplaceSel(L"later",TRUE);
        check(stale_capture && !workspace->TextInputCurrent(*stale_capture,large_path.wstring(),&error),L"background preparation is rejected after the native source generation changes");
        const std::string oversized(33 * 1024 * 1024,'x'); large_editor->text_.Call(SCI_SETTEXT,0,reinterpret_cast<LPARAM>(oversized.c_str())); const auto oversized_generation = large_editor->text_.Generation();
        check(!workspace->CaptureTextInput(large_path.wstring(),&error) && large_editor->text_.Call(SCI_GETLENGTH) == static_cast<std::intptr_t>(oversized.size()) && large_editor->text_.Generation() == oversized_generation,L"oversized snapshots are rejected before copying native text and never trim the draft");
        workspace->MarkPreferencesDirty(); workspace->preferences_failed_ = true; const auto window = workspace->GetSafeHwnd();
        reliability_answer_ = IDCANCEL; workspace->OnClose(); check(::IsWindow(window) && large_editor->text_.Generation() == oversized_generation,L"cancel exit after preference failure retains every open draft");
        reliability_answer_ = IDNO; workspace->OnClose(); const auto close_end = GetTickCount64() + 5000;
        while (::IsWindow(window) && GetTickCount64() < close_end) { MSG message{}; while (::PeekMessage(&message,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&message); DispatchMessage(&message); } Sleep(1); }
        check(!::IsWindow(window) && LoadPreferences(ini,&loaded,&error) && loaded.font_tenths == 140,L"explicit exit without saving preferences closes without retry loops or rewriting settings");
        if (::IsWindow(window)) workspace->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr; reliability_owner_ = nullptr; ::KillTimer(nullptr,timer); return failures;
    }

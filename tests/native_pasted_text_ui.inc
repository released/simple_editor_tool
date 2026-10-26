    static int RunPastedText(const std::filesystem::path& root,const std::filesystem::path& screenshots = {}) {
        using namespace simple_editor::core;
        int failures = 0;
        const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
        auto* workspace = new CWorkspaceFrame(false); AfxGetApp()->m_pMainWnd = workspace;
        workspace->SetWindowPos(nullptr,0,0,1400,1000,SWP_NOACTIVATE | SWP_NOZORDER);
        workspace->SendMessage(WM_COMMAND,ID_WORKSPACE_PASTED_TEXT);
        check(workspace->sessions_.size() == 1,L"pasted-text menu opens a new comparison without choosing files");
        if (workspace->sessions_.empty()) { workspace->DestroyWindow(); return failures; }
        auto* frame = workspace->sessions_.back(); auto& view = frame->advanced_;
        check(frame->displayed_mode_ == 8 && view.mode_ == 2 && view.pasted_inputs_ && view.input_left_.GetSafeHwnd() && view.input_right_.GetSafeHwnd(),L"dedicated mode creates two native editable source panes");
        if (!view.input_left_.GetSafeHwnd() || !view.input_right_.GetSafeHwnd()) { workspace->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr; return failures; }
        check(!(frame->compare_left_path_.GetStyle() & WS_VISIBLE) && !(frame->compare_right_path_.GetStyle() & WS_VISIBLE) && (view.input_left_.GetStyle() & WS_VISIBLE),L"text inputs replace file path selectors");
        frame->OnCompareRun(); check(Wait(frame) && view.model_ && view.model_->left.empty() && view.model_->right.empty(),L"two empty sides compare successfully without disk inputs");
        const std::wstring a = L"alpha\r\n\x4E2D\x6587 A\nlast\r", b = L"alpha\r\n\x4E2D\x6587 B\nlast\r";
        view.input_left_.ReplaceSel(a.c_str(),TRUE); view.input_right_.ReplaceSel(b.c_str(),TRUE);
        check(view.stale_ && view.rows_.GetItemCount() == 0 && view.HasPastedInputs(),L"native insert notifications invalidate old rows and retain both drafts");
        frame->OnCompareRun();
        check(frame->IsBusy() && view.input_left_.Call(SCI_GETREADONLY) && view.input_right_.Call(SCI_GETREADONLY),L"background comparison locks both input snapshots");
        const auto before = view.input_left_.Utf8(); view.input_left_.ReplaceSel(L"blocked",TRUE);
        check(view.input_left_.Utf8() == before,L"input mutations are rejected while comparison is running");
        check(Wait(frame) && view.model_ && view.model_->left.size() == 3 && view.model_->right.size() == 3 && !view.report_inputs_.size() && !frame->document_.IsOpen(),L"Unicode and mixed endings compare entirely in memory");
        if (!view.model_) { workspace->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr; return failures; }
        check(view.model_->left[0].ending == L"\r\n" && view.model_->left[1].ending == L"\n" && view.model_->left[2].ending == L"\r",L"comparison preserves original CRLF LF and CR endings");
        check(!view.input_left_.Call(SCI_GETREADONLY) && view.rows_.GetItemCount() == 3,L"completed comparison restores editing and exposes aligned differences");
        check(std::any_of(view.model_->blocks.begin(),view.model_->blocks.end(),[](const auto& block) { return block.changed; }),L"changed pasted text produces a real difference block");
        const auto output = view.RawOutputText(); view.input_left_.ReplaceSel(L"extra",TRUE);
        check(view.stale_ && view.rows_.GetItemCount() == 0 && view.RawOutputText() == output,L"editing a source clears stale differences while retaining merge output");
        view.input_left_.Undo(); check(view.input_left_.Utf8() == before,L"each pasted input retains its own native Undo");
        view.input_left_.Redo(); check(view.input_left_.Utf8() != before,L"each pasted input retains its own native Redo"); view.input_left_.Undo();
        frame->OnCompareSwap();
        check(view.input_left_.Utf8() == CCodeEditor::ToUtf8(b) && view.input_right_.Utf8() == before && view.input_right_.CanUndo(),L"swap exchanges the native buffers and their Undo histories");
        frame->OnCompareSwap(); check(view.input_left_.Utf8() == before && view.input_right_.Utf8() == CCodeEditor::ToUtf8(b),L"repeated swap restores the original sources without conversion");
        check(workspace->CaptureWorkspace().empty(),L"workspace capture never serializes pasted text or synthetic file paths");
        const auto put = [&](const std::wstring& left,const std::wstring& right) { view.input_left_.SetWindowTextW(left.c_str()); view.input_right_.SetWindowTextW(right.c_str()); view.OnPastedInputChanged(); };
        put(L"",L"only on right\n"); frame->OnCompareRun();
        check(Wait(frame) && view.model_ && view.model_->left.empty() && view.model_->right.size() == 1,L"empty left input is valid and reports right-side additions");
        put(L"Case \r\n",L"case\n"); view.ignore_case_.SetCheck(BST_CHECKED); view.ignore_space_.SetCheck(BST_CHECKED); view.ignore_eol_.SetCheck(BST_CHECKED); frame->OnCompareRun();
        check(Wait(frame) && view.model_ && std::none_of(view.model_->blocks.begin(),view.model_->blocks.end(),[](const auto& block) { return block.changed && block.important; }),L"pasted inputs use existing ignore-case whitespace and EOL options");
        view.ignore_case_.SetCheck(BST_UNCHECKED); view.ignore_space_.SetCheck(BST_UNCHECKED); view.ignore_eol_.SetCheck(BST_UNCHECKED);
        std::wstring large,other; for (unsigned i = 0; i < 60000; ++i) { const auto line = L"line " + std::to_wstring(i) + L" \x4E2D\x6587\n"; large += line; other += i == 30000 ? L"changed middle\n" : line; }
        put(large,other); frame->OnCompareRun();
        check(Wait(frame) && view.model_ && view.model_->left.size() == 60000 && view.model_->right.size() == 60000 && view.rows_.GetItemCount() == 60000,L"large pasted blocks beyond native Edit limits retain all 60000 aligned lines");
        check(view.input_left_.Utf8() == CCodeEditor::ToUtf8(large) && view.input_right_.Utf8() == CCodeEditor::ToUtf8(other),L"large comparison leaves complete source buffers unchanged");
        const auto original_font = view.input_left_.GetFont()->GetSafeHandle(); frame->SetTabZoom(2,150);
        check(view.input_left_.GetFont()->GetSafeHandle() != original_font && view.input_left_.GetFont()->GetSafeHandle() == view.output_.GetFont()->GetSafeHandle() && view.input_left_.Utf8() == CCodeEditor::ToUtf8(large),L"comparison zoom updates both input fonts without changing content");
        frame->compare_mode_.SetCurSel(2); frame->OnCompareModeChanged(); frame->compare_mode_.SetCurSel(8); frame->OnCompareModeChanged();
        check(view.pasted_inputs_ && view.input_left_.Utf8() == CCodeEditor::ToUtf8(large) && view.input_right_.Utf8() == CCodeEditor::ToUtf8(other),L"switching file/text modes preserves pasted drafts");
        put(L"cancel left\n",L"cancel right\n"); frame->OnCompareRun(); frame->OnCompareCancel();
        check(Wait(frame) && view.stale_ && view.rows_.GetItemCount() == 0 && view.input_left_.Utf8() == "cancel left\n",L"cancellation retains input drafts and never publishes cancelled differences");
        put(std::wstring(200001,L'\n'),L""); frame->OnCompareRun(); check(Wait(frame) && view.stale_,L"over-limit line count is rejected without adopting stale results");
        CString status; view.status_.GetWindowTextW(status); check(status.Find(L"200,000") >= 0,L"line-limit failure explains the supported quota");
        const std::string oversized(16 * 1024 * 1024 + 1,'x'); view.input_left_.Call(SCI_SETTEXT,0,reinterpret_cast<LPARAM>(oversized.c_str())); view.OnPastedInputChanged(); frame->OnCompareRun();
        view.status_.GetWindowTextW(status); check(!frame->IsBusy() && view.input_left_.Call(SCI_GETLENGTH) == static_cast<LRESULT>(oversized.size()) && status.Find(L"16 MiB") >= 0,L"oversized paste is retained and explicitly rejected instead of silently truncated");
        put(L"",L""); const char nul[] = {'a',0,'b'}; view.input_left_.Call(SCI_ADDTEXT,sizeof(nul),reinterpret_cast<LPARAM>(nul)); frame->OnCompareRun(); view.status_.GetWindowTextW(status);
        check(!frame->IsBusy() && status.Find(L"NUL") >= 0,L"embedded NUL input cannot be silently dropped during comparison");
        put(L"left\n",L"right\n"); frame->OnCompareRun(); check(Wait(frame),L"valid pasted comparison recovers after quota and input errors");
        const auto saved = root / L"pasted-output.txt"; check(view.SaveTo(saved.wstring()) && Wait(frame) && ReadDisk(saved) == std::vector<std::uint8_t>({'l','e','f','t','\n'}),L"Save result writes only the explicitly chosen UTF-8 output");
        const auto timer = ::SetTimer(nullptr,0,20,[](HWND,UINT,UINT_PTR,DWORD) { const HWND dialog = TestDialog(); if (dialog && ::GetDlgItem(dialog,IDCANCEL)) ::PostMessageW(dialog,WM_COMMAND,IDCANCEL,0); });
        const bool closed = view.CanClose(); ::KillTimer(nullptr,timer);
        check(!closed && view.input_left_.Utf8() == "left\n",L"cancelling the pasted-input close warning preserves both sources");
        if (!screenshots.empty()) {
            put(L"alpha\n\x4E2D\x6587 A\nlast\n",L"alpha\n\x4E2D\x6587 B\nlast\n"); frame->OnCompareRun(); check(Wait(frame),L"visual fixture compares pasted Unicode inputs");
            std::filesystem::create_directories(screenshots); workspace->ShowWindow(SW_SHOWNOACTIVATE); workspace->RedrawWindow(nullptr,nullptr,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
            CRect bounds; workspace->GetWindowRect(bounds); CImage image; image.Create(bounds.Width(),bounds.Height(),32); const HDC dc = image.GetDC(); const auto drawn = ::PrintWindow(workspace->GetSafeHwnd(),dc,0); image.ReleaseDC();
            check(drawn && SUCCEEDED(image.Save((screenshots / L"pasted-text.png").c_str())),L"capture the owned pasted-text comparison window"); workspace->ShowWindow(SW_HIDE);
        }
        Write(root / L"editor.txt",{'d','r','a','f','t'}); workspace->OpenPath((root / L"editor.txt").wstring()); auto* editor = workspace->sessions_.back(); const auto count = workspace->sessions_.size();
        editor->compare_mode_.SetCurSel(8); editor->OnCompareModeChanged();
        check(workspace->sessions_.size() == count + 1 && workspace->sessions_.back()->comparison_session_ && editor->document_.Path() == (root / L"editor.txt").wstring(),L"choosing pasted mode from a file editor creates an independent session");
        auto* switched = workspace->sessions_.back(); switched->compare_mode_.SetCurSel(2); switched->OnCompareModeChanged();
        SetPathComboText(switched->compare_left_path_,(root / L"editor.txt").wstring()); SetPathComboText(switched->compare_right_path_,(root / L"editor.txt").wstring()); switched->OnCompareRun();
        check(Wait(switched) && switched->document_.IsOpen(),L"file-comparison transition fixture owns a read-only source preview");
        switched->compare_mode_.SetCurSel(8); switched->OnCompareModeChanged();
        check(!switched->document_.IsOpen() && editor->document_.IsOpen() && editor->document_.Path() == (root / L"editor.txt").wstring(),L"switching to pasted mode releases obsolete file previews while preserving the source editor");
        put(L"",L""); workspace->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr;
        return failures;
    }

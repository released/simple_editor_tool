    static int RunSelectionHighlights(const std::filesystem::path& root,const std::filesystem::path& screenshots = {}) {
        int failures = 0;
        const auto check = [&](bool ok,const wchar_t* name) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << name << L'\n'; if (!ok) ++failures; };
        auto* workspace = new CWorkspaceFrame(false); AfxGetApp()->m_pMainWnd = workspace;
        workspace->preferences_path_ = root / L"selection-synthetic.ini";
        workspace->SetWindowPos(nullptr,0,0,1400,920,SWP_NOACTIVATE | SWP_NOZORDER); workspace->ShowWindow(SW_SHOW);
        std::string original = "Transfer fooTransferBar Transfer\ntransfer TRANSFER\na.b axb a.b\n";
        for (int n = 0; n < 12; ++n) original += "Project" + std::to_string(n) + "Transfer = 'synthetic/Transfer/Project';\n";
        const auto path = root / L"selection-highlights.txt"; Write(path,{original.begin(),original.end()});
        workspace->OpenPath(path.wstring()); auto* frame = workspace->sessions_.back(); auto& edit = frame->text_;
        const auto pump = [] { MSG message{}; while (PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&message); DispatchMessageW(&message); } };
        const auto settle = [&](CCodeEditor& editor) {
            const auto started = GetTickCount64();
            do { pump(); if (GetTickCount64() - started > 180 && !editor.occurrence_queued_ && editor.occurrence_complete_) return true; Sleep(1); } while (GetTickCount64() - started < 4000);
            return false;
        };
        const auto mark = [](CCodeEditor& editor,LRESULT at,unsigned indicator = 14) { return editor.Call(SCI_INDICATORVALUEAT,indicator,at); };
        edit.Call(SCI_SETEMPTYSELECTION,edit.Call(SCI_GETLENGTH)); edit.ReplaceSel(L"draft",TRUE); edit.SetFocus(); edit.SetSel(0,8);
        edit.Call(SCI_SETINDICATORCURRENT,8); edit.Call(SCI_SETINDICATORVALUE,19); edit.Call(SCI_INDICATORFILLRANGE,9,3);
        edit.Call(SCI_SETINDICATORCURRENT,13); edit.Call(SCI_INDICATORFILLRANGE,12,1);
        edit.Call(SCI_SETINDICATORCURRENT,8); edit.Call(SCI_SETINDICATORVALUE,19);
        edit.Call(SCI_SETTARGETRANGE,3,17); edit.Call(SCI_SETSEARCHFLAGS,SCFIND_MATCHCASE | SCFIND_WHOLEWORD);
        const auto text = edit.Utf8(); const auto generation = edit.Generation(),revision = frame->revision_;
        check(settle(edit) && mark(edit,12) && mark(edit,24) && !mark(edit,34) && !mark(edit,43),L"single selection highlights exact case-sensitive substrings, including inside identifiers");
        check(edit.Utf8() == text && edit.Generation() == generation && frame->revision_ == revision && edit.Call(SCI_GETANCHOR) == 0 && edit.Call(SCI_GETCURRENTPOS) == 8 && edit.CanUndo(),L"automatic indicators retain source, caret, selection, versions and native Undo");
        check(edit.Call(SCI_GETINDICATORCURRENT) == 8 && edit.Call(SCI_GETINDICATORVALUE) == 19 && edit.Call(SCI_GETTARGETSTART) == 3 && edit.Call(SCI_GETTARGETEND) == 17 && edit.Call(SCI_GETSEARCHFLAGS) == (SCFIND_MATCHCASE | SCFIND_WHOLEWORD),L"highlighting preserves the native search target, flags and current indicator state");
        check(mark(edit,9,8) == 19 && mark(edit,12,13),L"search/review marks and Unicode notice indicators are independent");
        check(ReadDisk(path) == std::vector<std::uint8_t>(original.begin(),original.end()),L"selection highlighting never saves a draft");
        edit.SetSel(8,0); check(settle(edit) && mark(edit,12),L"reverse drag selection keeps the same literal highlights");
        const auto dotted = text.find("a.b"); edit.Call(SCI_SETSEL,dotted,dotted + 3);
        check(settle(edit) && mark(edit,dotted + 8) && !mark(edit,dotted + 4) && !mark(edit,12),L"punctuation is literal and a new selection removes the previous query marks");
        edit.SetSel(0,0); check(settle(edit) && !mark(edit,dotted) && !mark(edit,12),L"collapsing the selection clears automatic highlights");
        edit.SetSel(8,9); check(settle(edit) && edit.occurrence_text_.empty(),L"whitespace-only selections do not flood the document with marks");
        edit.SetSel(0,40); check(settle(edit) && edit.occurrence_text_.empty(),L"multiline selection does not start automatic occurrence scanning");
        edit.SetSel(0,8); edit.Call(SCI_ADDSELECTION,32,24); check(settle(edit) && edit.occurrence_text_.empty(),L"multiple and rectangular editing selections stay independent from automatic highlighting"); edit.Call(SCI_CLEARSELECTIONS);
        edit.SetSel(0,8); edit.Call(SCI_SETSELECTIONMODE,SC_SEL_RECTANGLE); check(settle(edit) && edit.occurrence_text_.empty(),L"rectangular selections never become an ambiguous literal query"); edit.Call(SCI_SETSELECTIONMODE,SC_SEL_STREAM);
        edit.SetWindowTextW(L"中文\U0001F680 / 中文\U0001F680 / 中\U0001F680"); edit.SetSel(0,4);
        check(settle(edit) && mark(edit,13) && !mark(edit,26),L"Unicode literal matching keeps full CJK and supplementary-character boundaries");
        edit.SetWindowTextW(L"aaaaa"); edit.SetSel(0,3); check(settle(edit) && mark(edit,4),L"overlapping exact matches are highlighted");
        edit.SetWindowTextW((std::wstring(1025,L'x') + L" tail").c_str()); edit.SetSel(0,1025);
        check(settle(edit) && edit.occurrence_text_.empty(),L"oversized selections are bounded without trimming or changing source");

        edit.SetWindowTextW(CCodeEditor::FromUtf8(original).c_str()); edit.SetSel(0,8); edit.SetReadOnly(TRUE);
        check(settle(edit) && mark(edit,12),L"read-only text supports the same visual occurrence marks"); edit.SetReadOnly(FALSE);
        const auto style_before = edit.Call(SCI_INDICGETFORE,14); edit.SetTheme(0); check(settle(edit) && edit.Call(SCI_INDICGETFORE,14) != style_before && mark(edit,12),L"light/dark palettes keep visible automatic marks");
        edit.SetTheme(2); check(settle(edit) && edit.Call(SCI_INDICGETFORE,14) == GetSysColor(COLOR_HIGHLIGHT) && mark(edit,12),L"system contrast uses the system highlight color"); edit.SetTheme(1);
        frame->SetTabZoom(1,150); check(settle(edit) && mark(edit,12),L"font zoom refreshes visible occurrence coverage"); frame->SetTabZoom(1,100);
        edit.SetWrap(true); check(settle(edit) && mark(edit,12),L"wrapped display rows retain exact source occurrence marks"); edit.SetWrap(false);
        frame->ToggleTextClone(); auto& clone = frame->text_clone_; clone.SetFocus(); clone.SetSel(9,12);
        check(settle(clone) && mark(clone,9) && !mark(clone,24),L"focused shared view owns the current selection's document indicators");
        edit.SetFocus(); check(settle(edit) && mark(edit,24) && edit.occurrence_text_ == "Transfer",L"returning to the original view restores its own selected query"); frame->ToggleTextClone();
        const auto other = root / L"other.txt"; Write(other,{'o','t','h','e','r',' ','o','t','h','e','r'}); workspace->OpenPath(other.wstring()); auto* second = workspace->sessions_.back(); second->text_.SetFocus(); second->text_.SetSel(0,5);
        check(settle(second->text_) && mark(second->text_,6) && mark(edit,24),L"separate tabs retain independent document indicators");
        workspace->ActivateSession(0); edit.SetFocus(); check(settle(edit) && mark(edit,24),L"tab activation restores the current document's selected occurrences");
        edit.SetSel(0,8); edit.ReplaceSel(L"Changed",TRUE); check(settle(edit) && !mark(edit,24),L"editing clears stale automatic positions before repaint"); edit.Undo(); edit.SetSel(0,8);
        check(settle(edit) && mark(edit,24),L"Undo can restore the source and its selected occurrence marks");

        const std::size_t distant = 200000; std::string long_line = "Transfer" + std::string(distant - 8,'x') + "Transfer tail";
        edit.SetWindowTextW(CCodeEditor::FromUtf8(long_line).c_str()); edit.SetSel(0,8); edit.Call(SCI_SETXOFFSET,0);
        check(settle(edit) && !mark(edit,distant),L"a distant long-line match is deferred instead of scanning the entire source on selection");
        const auto cell = edit.Call(SCI_TEXTWIDTH,STYLE_DEFAULT,reinterpret_cast<LPARAM>("x")); edit.Call(SCI_SETSCROLLWIDTH,static_cast<WPARAM>((distant + 100) * cell)); edit.Call(SCI_SETXOFFSET,(distant - 12) * cell);
        check(settle(edit) && mark(edit,distant),L"horizontal scrolling updates far-away matches on a long physical line");
        std::intptr_t covered = 0; for (const auto& range : edit.occurrence_ranges_) covered += range.second - range.first;
        check(covered < 32768 && edit.occurrence_ranges_.size() < 100,L"visible scanning bounds source bytes and range storage independently of physical line length");
        std::wstring tall = L"Transfer\r\n"; for (int n = 0; n < 10000; ++n) tall += L"filler row\r\n"; tall += L"Transfer\r\n";
        edit.SetWindowTextW(tall.c_str()); edit.SetSel(0,8); edit.Call(SCI_SETXOFFSET,0); edit.Call(SCI_SETFIRSTVISIBLELINE,10000);
        const auto bottom_match = edit.Utf8().rfind("Transfer"); check(settle(edit) && mark(edit,bottom_match),L"vertical scrolling marks occurrences anywhere in the current document");
        edit.Call(SCI_SETFIRSTVISIBLELINE,0); edit.Call(SCI_HIDELINES,1,10000); check(settle(edit) && mark(edit,bottom_match),L"visible matches beyond a large folded region use physical source positions");
        covered = 0; for (const auto& range : edit.occurrence_ranges_) covered += range.second - range.first;
        check(covered < 32768,L"folded source gaps are not synchronously scanned as one giant range"); edit.Call(SCI_SHOWLINES,1,10000);
        {
            CCodeEditor source; check(source.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL,CRect(20,200,700,700),frame,62000),L"create read-only comparison presentation fixture");
            source.SetFont(&frame->text_font_); source.SetSourcePresentation(true); source.SetWindowTextW(L"Transfer Transfer"); source.SetReadOnly(TRUE); source.SetFocus(); source.SetSel(0,8);
            check(settle(source) && mark(source,source.NativePosition(9)) && !mark(source,0),L"source presentation excludes its synthetic header while preserving physical match positions"); source.DestroyWindow();
        }
        edit.SetFocus();
        edit.Call(SCI_SETFIRSTVISIBLELINE,0); edit.SetWrap(false); edit.SetWindowTextW(CCodeEditor::FromUtf8(original).c_str()); edit.SetSel(0,8); check(settle(edit),L"restore the synthetic demonstration document");
        const auto idle_generation = edit.Generation(),idle_revision = frame->revision_; const auto idle_at = edit.occurrence_at_; pump(); Sleep(200); pump();
        check(settle(edit) && !edit.occurrence_queued_ && edit.occurrence_at_ == idle_at && edit.Generation() == idle_generation && frame->revision_ == idle_revision,L"indicator repaint notifications settle without rescan loops or document dirtying");
        if (!screenshots.empty()) { std::filesystem::create_directories(screenshots); check(ExpansionCapture(*workspace,screenshots / L"selection-highlight.png"),L"capture native selected-word and other-occurrence appearance"); }
        workspace->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr;
        return failures;
    }

    static int RunSideMerge(const std::filesystem::path& root,const std::filesystem::path& screenshot = {}) {
        using namespace simple_editor::core;
        int failures = 0;
        const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
        const auto a = root / L"left.cpp",b = root / L"right.cpp";
        const auto write = [&](const auto& path,const char* text) { const std::string value(text); Write(path,{value.begin(),value.end()}); };
        write(a,"top\nleft\nsame\nleft-end\n"); write(b,"top\r\nright\r\nsame\r\nright-end\r\n");
        const auto disk_a = ReadDisk(a),disk_b = ReadDisk(b);
        auto* workspace = new CWorkspaceFrame(false); AfxGetApp()->m_pMainWnd = workspace;
        workspace->preferences_path_ = root / L"synthetic.ini";
        workspace->SetWindowPos(nullptr,0,0,1360,940,SWP_NOZORDER | SWP_NOACTIVATE); workspace->ShowWindow(SW_SHOWNOACTIVATE);
        workspace->OpenPath(a.wstring()); auto* left = workspace->sessions_.back();
        workspace->NewComparisonInputs(a.wstring(),b.wstring(),L"",2,true); auto* frame = workspace->sessions_.back(); auto& view = frame->advanced_;
        check(Wait(frame) && view.model_,L"two-way side merge opens using file and editor snapshots");
        view.ignore_eol_.SetCheck(BST_CHECKED); frame->OnCompareRun(); check(Wait(frame),L"comparison can ignore EOL while copies preserve destination newline style");
        CRect l,m,r; view.source_left_.GetWindowRect(l); view.rows_.GetWindowRect(m); view.source_right_.GetWindowRect(r);
        check(l.right <= m.left && m.right <= r.left && l.top == r.top && l.bottom == r.bottom && m.bottom == l.bottom,L"line-number list sits between equal-height source panes");
        CRect bounds; view.GetClientRect(bounds);
        check(l.Height() > bounds.Height() / 2 && !view.output_.IsWindowVisible(),L"source panes use most height with merged output initially collapsed");
        check(view.rows_.GetColumnWidth(3) == 0 && view.rows_.GetColumnWidth(5) == 0 && view.rows_.GetColumnWidth(2) > 0 && view.rows_.GetColumnWidth(4) > 0,L"middle list displays left/right line numbers without duplicate source columns");
        view.review_focus_ = nullptr; view.next_.SetFocus(); check(view.SearchEditor() == &view.source_left_,L"collapsed-output Find defaults to the visible left source");
        const auto command = [&](UINT id) { if (auto* button = view.GetDlgItem(id)) button->SetFocus(); view.SendMessage(WM_COMMAND,id); return Wait(frame); };
        command(ID_ADV_NEXT); const int first = view.rows_.GetNextItem(-1,LVNI_SELECTED); const auto first_block = first >= 0 ? view.text_rows_[view.visible_[first]].block : SIZE_MAX;
        command(ID_ADV_NEXT); const int next = view.rows_.GetNextItem(-1,LVNI_SELECTED);
        check(first >= 0 && next >= 0 && view.text_rows_[view.visible_[next]].block != first_block,L"next difference skips to a different block");
        command(ID_ADV_PREVIOUS); const int previous = view.rows_.GetNextItem(-1,LVNI_SELECTED);
        check(previous >= 0 && view.text_rows_[view.visible_[previous]].block == first_block,L"previous difference returns to the prior block");
        view.source_left_.GoToLine(2); view.SourceChanged(1); const int clicked = view.rows_.GetNextItem(-1,LVNI_SELECTED);
        check(clicked >= 0 && view.text_rows_[view.visible_[clicked]].left == 1,L"source caret selection chooses its corresponding middle line without rewriting text");
        check(command(ID_ADV_BLOCK_RIGHT) && workspace->sessions_.size() == 3 && workspace->sessions_[workspace->active_session_] == frame,L"block copy creates an inactive target editor and keeps comparison active");
        CMainFrame* right = nullptr; for (auto* candidate : workspace->sessions_) if (!candidate->comparison_session_ && candidate->document_.Path() == b.wstring()) right = candidate;
        check(right && right->text_dirty_ && right->text_.Utf8().find("left\r\n") != std::string::npos && right->text_.Utf8().find("right-end") != std::string::npos,L"selected block changes only that target range and retains the other difference");
        check(ReadDisk(a) == disk_a && ReadDisk(b) == disk_b && right && right->text_.CanUndo(),L"copy keeps disk inputs unchanged and adds target native Undo");
        check(!view.stale_ && view.model_ && view.source_right_.Utf8().find("left\r\n") != std::string::npos,L"copy recomputes differences from the unsaved target draft");
        check(command(ID_ADV_UNDO) && right && right->text_.Utf8().find("right\r\n") != std::string::npos,L"comparison Undo restores the target draft in one action");
        check(command(ID_ADV_REDO) && right && right->text_.Utf8().find("left\r\n") != std::string::npos,L"comparison Redo reapplies the block");
        if (right) { right->user_read_only_ = true; const auto before = right->text_.Utf8(); command(ID_ADV_WHOLE_RIGHT); check(right->text_.Utf8() == before,L"read-only target rejects side copy"); right->user_read_only_ = false; }
        TextSideCopy stale; stale.to_right = true; stale.left_path = a.wstring(); stale.right_path = b.wstring(); stale.left = L"outdated"; stale.right = L"outdated"; stale.replacement = L"lost"; std::wstring error;
        const auto retained = right ? right->text_.Utf8() : std::string();
        check(!workspace->ApplyComparisonCopy(stale,&error) && right && right->text_.Utf8() == retained,L"stale source/target cannot apply old block positions");
        check(command(ID_ADV_WHOLE_RIGHT) && right && right->text_.Utf8() == left->text_.Utf8() && ReadDisk(b) == disk_b,L"whole left-to-right copy changes only the unsaved destination");
        check(view.model_ && view.source_left_.Utf8() == view.source_right_.Utf8(),L"whole copy produces equal displayed text while retaining destination EOL");
        command(ID_ADV_UNDO);
        command(ID_ADV_NEXT); check(command(ID_ADV_BLOCK_LEFT) && left->text_dirty_ && left->text_.CanUndo() && ReadDisk(a) == disk_a,L"block right-to-left copy preserves the left original until Save");
        command(ID_ADV_UNDO);
        check(command(ID_ADV_WHOLE_LEFT) && left->text_.Utf8() == right->text_.Utf8() && ReadDisk(a) == disk_a,L"whole right-to-left copy updates the left draft");
        command(ID_ADV_UNDO);
        view.show_output_button_.SetCheck(BST_CHECKED); command(ID_ADV_SHOW_OUTPUT);
        check(view.output_.IsWindowVisible() && view.source_left_.IsWindowVisible(),L"merged output can be expanded without losing side sources");
        view.show_output_button_.SetCheck(BST_UNCHECKED); command(ID_ADV_SHOW_OUTPUT);
        if (!screenshot.empty()) check(ExpansionCapture(*workspace,screenshot),L"capture centered line list and full-height source panes");
        // Save the left draft explicitly, then verify the right-side draft independently.
        command(ID_ADV_WHOLE_LEFT); command(ID_ADV_SAVE_LEFT); check(Wait(left) && ReadDisk(a) != disk_a && ReadDisk(b) == disk_b,L"Save left commits only the chosen document through the normal guarded writer");
        view.source_right_.SetFocus(); frame->OnSave(); check(right && Wait(right) && ReadDisk(b) != disk_b,L"Ctrl+S on the right source commits its draft independently");
        for (int width : {900,1600}) { workspace->SetWindowPos(nullptr,0,0,width,940,SWP_NOZORDER | SWP_NOACTIVATE); view.source_left_.GetWindowRect(l); view.rows_.GetWindowRect(m); view.source_right_.GetWindowRect(r); check(l.Width() > 0 && r.Width() > 0 && l.right <= m.left && m.right <= r.left,L"resized comparison retains non-overlapping side panes"); }
        view.SetZoom(150); view.source_left_.GetWindowRect(l); view.rows_.GetWindowRect(m); view.source_right_.GetWindowRect(r); check(l.right <= m.left && m.right <= r.left,L"zoom retains centered line-number geometry");
        view.SetZoom(100);
        const auto unicode = root / L"unicode.txt",legacy = root / L"big5.txt";
        const auto utf8 = CCodeEditor::ToUtf8(L"Unicode \U0001F680\n"); Write(unicode,{utf8.begin(),utf8.end()}); write(legacy,"plain\n");
        workspace->OpenPath(legacy.wstring()); auto* legacy_editor = workspace->sessions_.back(); legacy_editor->requested_encoding_ = TextEncoding::Big5; legacy_editor->RefreshText();
        workspace->NewComparisonInputs(unicode.wstring(),legacy.wstring(),L"",2,true); auto* encoded_frame = workspace->sessions_.back(); auto& encoded_view = encoded_frame->advanced_; check(Wait(encoded_frame),L"independent legacy encoding fixture opens");
        encoded_view.encoding_right_.SetCurSel(static_cast<int>(TextEncoding::Big5)); encoded_frame->OnCompareRun(); check(Wait(encoded_frame),L"right source uses its explicit Big5 codec");
        const auto legacy_before = legacy_editor->text_.Utf8(); encoded_view.CopySide(true,true);
        check(Wait(encoded_frame) && legacy_editor->text_.Utf8() == legacy_before && !legacy_editor->text_.CanUndo() && ReadDisk(legacy) == std::vector<std::uint8_t>({'p','l','a','i','n','\n'}),L"unrepresentable target text rejects copy before mutation or Undo creation");
        for (auto* target : workspace->sessions_) { target->text_dirty_ = false; target->text_.SetModify(FALSE); target->document_.Close(); target->advanced_.dirty_ = false; }
        workspace->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr; return failures;
    }

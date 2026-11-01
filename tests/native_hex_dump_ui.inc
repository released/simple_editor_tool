    static int RunHexDump(const std::filesystem::path& root,const std::filesystem::path& screenshot = {}) {
        using namespace simple_editor::core; using namespace simple_editor::ui;
        int failures = 0; const auto check = [&](bool ok,const wchar_t* name) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << name << L'\n'; if (!ok) ++failures; };
        check(PreferHexFileView(L"firmware.hex") && PreferHexFileView(L"firmware.HEX") && PreferHexFileView(L"firmware.hEx") && !PreferHexFileView(L"hex.txt") && !PreferHexFileView(L"file.hex.txt"),L"only the final .hex extension selects the filename-based Hex view, without case sensitivity");
        check(FormatHexAddress(0,8) == L"0000_0000" && FormatHexAddress(0x7B000,8) == L"0007_B000" && FormatHexAddress(0x100000020ULL,16) == L"0000_0001_0000_0020" && FormatHexAddress(UINT64_MAX,16) == L"FFFF_FFFF_FFFF_FFFF",L"dump addresses group hexadecimal digits in fours without truncating 64-bit values");
        std::wstring error; check(InitializeLocalization(std::filesystem::current_path() / L"src" / L"res",L"tw",&error),L"initialize native Hex dump localization");
        auto* workspace = new CWorkspaceFrame(false); AfxGetApp()->m_pMainWnd = workspace;
        workspace->SetWindowPos(nullptr,0,0,(std::min)(1600,GetSystemMetrics(SM_CXSCREEN)-40),(std::min)(880,GetSystemMetrics(SM_CYSCREEN)-80),SWP_NOZORDER | SWP_NOACTIVATE); workspace->ShowWindow(SW_SHOWNOACTIVATE);
        const std::string literal = "63D9025DED3D3F19552B1A72CCB6F4C6\n";
        const auto text_hex = root / L"key.hex",text = root / L"key.hex.txt",upper = root / L"layout.HEX";
        Write(text_hex,{literal.begin(),literal.end()}); Write(text,{literal.begin(),literal.end()});
        workspace->OpenPath(text_hex.wstring()); auto* key = workspace->sessions_.back();
        check(key->active_tab_ == 0 && key->text_stale_ && key->hex_.BytesPerRow() == 32 && Read(key) == std::vector<std::uint8_t>(literal.begin(),literal.end()),L"a printable .hex file opens directly as a 32-byte dump of the original bytes, without decoding or rewriting its text");
        key->ShowPage(1); const auto generation = key->text_.Generation(); workspace->OpenPath(text_hex.wstring());
        check(workspace->sessions_.size() == 1 && key->active_tab_ == 1 && key->text_.Utf8() == CCodeEditor::ToUtf8(TextForEditControl(std::wstring(literal.begin(),literal.end()))) && key->text_.Generation() == generation,L"explicit Text choice and existing-session activation retain the original Hex text buffer");
        workspace->OpenPath(text.wstring()); check(workspace->sessions_.back()->active_tab_ == 1,L"hexadecimal-looking text with another extension retains normal Text opening");
        std::vector<std::uint8_t> bytes(1024); for (std::size_t i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<std::uint8_t>(i);
        Write(upper,bytes); workspace->OpenPath(upper.wstring()); auto* frame = workspace->sessions_.back(); auto& view = frame->hex_;
        check(frame->active_tab_ == 0 && view.BytesPerRow() == 32 && frame->compare_left_.BytesPerRow() == 16 && frame->compare_right_.BytesPerRow() == 16,L"new file Hex dumps default to 32 bytes while side-by-side comparison widths stay independent");
        check(view.SetLayout(32,8,0x7B000) && view.AddressDigits() == 8 && view.HexColumn(0) == HexAddressColumns(8)+2,L"the address separator occupies its own column before the first byte");
        const auto unchanged = Read(frame);
        for (const auto row_width : {8u,16u,32u}) {
            view.SetLayout(row_width,8,0x7B000); view.SetTopRow(0);
            std::uint64_t offset = 0; bool ascii = false; int nibble = -1; const int column = 3;
            check(view.HitTest(CPoint(8+view.HexColumn(column)*view.char_width_+1,view.row_height_+1),&offset,&ascii,&nibble) && offset == row_width+column && !ascii && nibble == 0,L"Hex hit testing remains tied to physical bytes with grouped addresses and custom row widths");
            check(view.HitTest(CPoint(8+(view.HexColumn(row_width)+2+column)*view.char_width_+1,view.row_height_+1),&offset,&ascii,&nibble) && offset == row_width+column && ascii,L"ASCII hit testing remains aligned with the corresponding Hex byte");
        }
        view.SetLayout(32,8,0x100000000ULL);
        check(view.AddressDigits() == 16 && view.HexColumn(0) == HexAddressColumns(16)+2 && Read(frame) == unchanged,L"64-bit display bases reserve every address column without changing source bytes");
        view.SetLayout(32,8,0x7B000); view.SetTopRow(0); const auto edit = CPoint(8+view.HexColumn(17)*view.char_width_+1,view.row_height_+1);
        view.SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(edit.x,edit.y)); view.SendMessage(WM_LBUTTONUP,0,MAKELPARAM(edit.x,edit.y)); view.OnChar('A',1,0);
        check(Read(frame)[49] == 0xA1 && ReadDisk(upper) == bytes,L"editing a painted byte uses its physical offset and does not implicitly save"); frame->OnUndo();
        check(Read(frame) == bytes,L"one Undo restores the edited byte after the layout change");
        CHexView::ClipboardContent clip; view.clipboard_access = [&](bool writing,CHexView::ClipboardContent& value) { if (writing) clip = value; return writing; }; view.SelectRange(32,2);
        check(view.Clipboard(ID_HEX_COPY) && clip.bytes == std::vector<std::uint8_t>({32,33}) && clip.text == L"20 21",L"copy uses original byte values rather than formatted addresses or ASCII characters");
        view.ClearSelection(); view.GoTo(0); CRect client; view.GetClientRect(client); const int width = 16+(view.HexColumn(32)+32+3)*view.char_width_;
        if (width > client.Width()) frame->SetTabZoom(0,(std::max)(50,100*(client.Width()-24)/width/10*10));
        std::wcout << L"METRIC dump_bytes_per_row=" << view.BytesPerRow() << L" screenshot_zoom=" << frame->zoom_percent_[0] << L" viewport_width=" << client.Width() << L'\n';
        if (!screenshot.empty()) check(ExpansionCapture(*workspace,screenshot),L"capture address, 32 Hex bytes and the matching character column in the native interface");
        WorkspaceEntry entry; entry.left = text_hex.wstring(); entry.page = 1; entry.hex_rows[0] = 8; entry.hex_groups[0] = 4; entry.hex_bases[0] = 0x8000;
        auto* restored = new CWorkspaceFrame(false); restored->RestoreWorkspace({entry});
        check(restored->sessions_.size() == 1 && restored->sessions_[0]->active_tab_ == 1 && restored->sessions_[0]->hex_.BytesPerRow() == 8 && restored->sessions_[0]->hex_.Grouping() == 4 && restored->sessions_[0]->hex_.BaseAddress() == 0x8000,L"explicit restored page, row width, grouping and base take precedence over new-open defaults");
        restored->DestroyWindow(); workspace->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr;
        auto* standalone = new CMainFrame(false); AfxGetApp()->m_pMainWnd = standalone; standalone->OpenPath(text_hex.wstring());
        check(standalone->active_tab_ == 0 && standalone->text_stale_,L"standalone open/reload callers use the same filename-based Hex routing"); standalone->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr; return failures;
    }

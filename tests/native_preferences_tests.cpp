#include "../src/core/portable_preferences.h"
#include "../src/core/text_codec.h"
#include <fstream>
#include <iostream>
using namespace simple_editor::core;
int RunPreferenceTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; }; std::wstring error; AppPreferences p,loaded;
    check(LoadPreferences(root / L"missing.ini",&loaded,&error) && loaded.font == L"Consolas" && !loaded.auto_restore && !loaded.recover_drafts,L"absent portable INI retains opt-out defaults");
    const auto path = root / L"preferences.ini"; { std::ofstream out(path,std::ios::binary); out << "; user comment\n[Vendor.Section]\r\nUnknown=keep=value\n[SimpleEditorTool.Preferences]\r\nFont=OldFont\n"; }
    p.font = L"Cascadia Mono"; p.font_tenths = 120; p.theme = 2; p.wrap = p.whitespace = p.auto_restore = true; p.zoom[1] = 140; p.tab_width = 8; p.recent = {L"D:\\資料\\檔案😀.txt"}; ParseShortcut(L"Ctrl+Alt+L",&p.shortcuts[0],&error);
    check(SavePreferences(path,p,&error) && LoadPreferences(path,&loaded,&error) && loaded.font == p.font && loaded.font_tenths == 120 && loaded.theme == 2 && loaded.wrap && loaded.whitespace && loaded.auto_restore && !loaded.recover_drafts && loaded.zoom[1] == 140 && loaded.recent == p.recent && loaded.shortcuts[0].key == 'L' && loaded.shortcuts[0].modifiers == 5,L"portable preferences roundtrip Unicode paths, view settings and custom shortcuts");
    BinaryDocument document; std::vector<std::uint8_t> bytes; std::wstring text; TextCodec codec; document.Open(path.wstring(),&error); document.Read(0,static_cast<std::size_t>(document.Size()),&bytes); DecodeText(bytes,TextEncoding::Auto,&text,&codec,&error); document.Close();
    check(text.find(L"; user comment\n[Vendor.Section]\r\nUnknown=keep=value\n") != std::wstring::npos,L"preference updates preserve unrelated values, comments and their original newline styles");
    check(SavePreferences(path,p,&error) && SavePreferences(path,p,&error),L"repeated preference writes remain loadable without duplicating owned keys");
    Shortcut shortcut; check(ParseShortcut(L"Ctrl+Shift+F12",&shortcut,&error) && shortcut.key == VK_F12 && shortcut.modifiers == 3 && FormatShortcut(shortcut) == L"Ctrl+Shift+F12",L"shortcut parser preserves modifier/function-key semantics");
    check(!ParseShortcut(L"Shift+A",&shortcut,&error) && !ParseShortcut(L"Ctrl+Ctrl+A",&shortcut,&error) && ParseShortcut(L"None",&shortcut,&error) && !shortcut.key,L"ambiguous typing chords are rejected and explicit disabled shortcuts are supported");
    auto invalid = p; invalid.shortcuts[1] = invalid.shortcuts[0]; check(!SavePreferences(path,invalid,&error) && LoadPreferences(path,&loaded,&error) && loaded.shortcuts[1].key == 'O',L"duplicate shortcut assignments fail without replacing valid settings");
    invalid = p; invalid.font_tenths = 1; check(!ValidatePreferences(invalid,&error),L"font and view bounds reject malformed preferences");
    const auto malformed = root / L"bad-preferences.ini"; { std::ofstream out(malformed); out << "[SimpleEditorTool.Preferences]\nTheme=99\n"; } check(!LoadPreferences(malformed,&loaded,&error),L"invalid preference fields are reported instead of silently applying unexpected values");
    return failures;
}

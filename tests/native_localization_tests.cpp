#include "../src/core/language_catalog.h"
#include "../src/core/portable_preferences.h"
#include <fstream>
#include <iostream>
using namespace simple_editor::core;
int RunLocalizationTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* name) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << name << L'\n'; if (!ok) ++failures; };
    std::wstring error; LanguageCatalog catalog; const auto source = std::filesystem::current_path() / L"src" / L"res";
    check(catalog.Load(source,&error) && catalog.Entries() >= 600,L"paired UTF-8 language catalogs load without a runtime dependency");
    check(catalog.Select(L"tw") && catalog.Translate(L"&File") == L"檔案(&F)" && catalog.Translate(L"Current document") == L"目前文件",L"Traditional Chinese translation resolves stable source keys");
    check(catalog.Translate(L"Processing... 42%. Esc cancels.") == L"正在處理… 42%。按 Esc 取消。",L"printf-derived message templates preserve formatted progress values");
    check(catalog.Translate(L"Output saved: D:\\data\\Open File.bin") == L"已儲存輸出：D:\\data\\Open File.bin",L"message-template parameters preserve paths and spaces verbatim");
    check(catalog.Translate(L"3 decoded bytes; 4 formatted UTF-8 bytes. Complete preview.") == L"解碼資料 3 位元組；格式化 UTF-8 輸出 4 位元組。完整預覽。",L"leading numeric status templates keep complete payload counts");
    check(catalog.Translate(L"Overwrite  Hex  Offset 0xA0  Selection 0x80 + 32 bytes") == L"覆寫  十六進位  位址 0xA0  選取範圍 0x80 + 32 位元組",L"Hex status translates labels while retaining original addresses and byte counts");
    check(catalog.Translate(L"Text comparison complete. 0 unresolved conflict(s). Select changed rows and Take a side, or edit the output. Save result keeps source tabs intact.") == L"文字比較完成。尚有 0 個未解決衝突。選取變更列後採用任一側，或編輯合併輸出。「儲存結果」不會改寫來源分頁。",L"complete comparison status uses one bounded parameter-preserving template");
    check(catalog.Translate(L"Unknown private detail") == L"Unknown private detail" && !catalog.Select(L"invalid") && catalog.Selected() == L"tw",L"unknown messages fall back to original text and invalid selection retains language");
    check(catalog.Select(L"en") && catalog.Translate(L"&File") == L"&File" && catalog.Translate(L"Processing... 42%. Esc cancels.") == L"Processing... 42%. Esc cancels.",L"English selection restores original interface and progress text");
    LanguageFile en,tw; check(ReadLanguageFile(source / L"lang_en.ini",&en,&error) && ReadLanguageFile(source / L"lang_tw.ini",&tw,&error) && en.strings.size() == tw.strings.size(),L"shipped catalogs have matching nonempty key sets");
    bool stable = true; for (const auto& item : en.strings) if (LanguageKey(item.second) != item.first || !tw.strings.count(item.first)) stable = false; check(stable,L"every shipped language key is stable for its canonical English text");
    const auto definition = L"[Language]\nVersion=1\nId=tw\nName=繁體中文\n[Strings]\n" + LanguageKey(L"Line\nTab\tSlash\\") + L"=行\\nTab\\t斜線\\\\\n";
    check(ParseLanguageFile(definition,&tw,&error) && tw.strings.begin()->second == L"行\nTab\t斜線\\",L"INI escapes retain newlines, tabs and literal backslashes");
    const auto old = tw.strings; check(!ParseLanguageFile(definition + LanguageKey(L"Line\nTab\tSlash\\") + L"=duplicate\n",&tw,&error) && tw.strings == old,L"duplicate catalog keys reject atomically without replacing prior entries");
    check(!ParseLanguageFile(L"[Language]\nVersion=2\nId=en\nName=English\n[Strings]\nS0000000000000000=a\n",&tw,&error) && !ParseLanguageFile(definition + L"S0000000000000001=bad\\q\n",&tw,&error),L"unsupported versions and unknown escapes are rejected");
    std::filesystem::create_directories(root / L"broken-language"); LanguageCatalog missing; check(!missing.Load(root / L"broken-language",&error) && missing.Translate(L"Open files...") == L"Open files...",L"missing catalogs preserve built-in usable English text");
    AppPreferences preferences; check(preferences.language == L"tw",L"first-run language defaults to user-selected Traditional Chinese");
    const auto path = root / L"locale-preferences.ini"; { std::ofstream file(path,std::ios::binary); file << "[private]\nkeep=unchanged\n[simpleeditortool.preferences]\nzoom1=130\n"; }
    check(LoadPreferences(path,&preferences,&error) && preferences.language == L"tw" && preferences.zoom[1] == 130,L"older preference INIs gain language default without losing other settings");
    preferences.language = L"en"; AppPreferences restored; check(SavePreferences(path,preferences,&error) && LoadPreferences(path,&restored,&error) && restored.language == L"en" && restored.zoom[1] == 130,L"language choice persists through portable settings with prior zoom retained");
    { std::ifstream file(path,std::ios::binary); const std::string text{std::istreambuf_iterator<char>(file),{}}; check(text.find("keep=unchanged") != std::string::npos,L"language persistence preserves unknown existing INI fields"); }
    preferences.language = L"unsupported"; check(!SavePreferences(path,preferences,&error) && LoadPreferences(path,&restored,&error) && restored.language == L"en",L"invalid preference language cannot replace the existing settings file");
    return failures;
}

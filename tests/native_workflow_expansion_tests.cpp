#include "../src/core/source_alignment.h"
#include "../src/core/diff_presentation.h"
#include "../src/core/text_byte_map.h"
#include "../src/core/unicode_notices.h"
#include "../src/core/leb128.h"
#include "../src/core/data_conversion.h"
#include "../src/core/compare_presets.h"
#include "../src/core/structured_compare.h"
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace simple_editor::core;
int RunWorkflowExpansionTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* text) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << text << L'\n'; if (!ok) ++failures; };
    std::atomic_bool cancel{false}; SaveControl control; control.cancelled = &cancel; std::wstring error;
    TextMergeModel model; DiffPresentation presentation; SourceGapPlan gaps;
    check(BuildTextMerge(L"",L"common\nend\n",L"lead\ncommon\ninside\nend\n",false,{},cancel,&model,&error) && BuildDiffPresentation(model,{},false,cancel,&presentation) && BuildSourceGaps(presentation.rows,{0,model.left.size(),model.right.size()},false,&gaps) && gaps[1].size() == 2 && gaps[1][0].after_line == 0 && gaps[1][0].lines == 1 && gaps[1][1].after_line == 1,L"source display gaps cover leading and middle insertions without changing original line indices");
    const auto old = gaps; if (!presentation.rows.empty()) presentation.rows[0].left = 4; check(!BuildSourceGaps(presentation.rows,{0,model.left.size(),model.right.size()},false,&gaps) && gaps[1].size() == old[1].size(),L"malformed alignment mapping preserves prior output");
    TextCodec codec; codec.encoding = TextEncoding::Utf8; codec.bom = true; TextByteRange map;
    const std::wstring raw = L"A\x4E2D\U0001F680\nZ\rQ\r\nE";
    check(MapTextByteRange(raw,codec,true,1,4,control,&map,&error) && map.byte_begin == 4 && map.byte_end == 11,L"UTF-8 Text to Hex mapping accounts for BOM, CJK and surrogate pairs");
    check(MapTextByteRange(raw,codec,false,4,11,control,&map,&error) && map.text_begin == 1 && map.text_end == 4,L"Hex to Text round trip maps complete UTF-8 characters");
    check(!MapTextByteRange(raw,codec,false,5,11,control,&map,&error) && !MapTextByteRange(raw,codec,false,0,3,control,&map,&error) && !MapTextByteRange(raw,codec,true,5,6,control,&map,&error),L"partial UTF-8, BOM bytes and virtual half-newlines are rejected");
    codec.encoding = TextEncoding::Utf16Be; check(MapTextByteRange(raw,codec,true,1,4,control,&map,&error) && map.byte_begin == 4 && map.byte_end == 10 && !MapTextByteRange(raw,codec,false,4,9,control,&map,&error),L"UTF-16 BE physical boundaries reject partial words");
    codec.encoding = TextEncoding::Big5; codec.code_page = 950; codec.bom = false; check(MapTextByteRange(L"A\x4E2D\n",codec,true,1,2,control,&map,&error) && map.byte_begin == 1 && map.byte_end == 3 && !MapTextByteRange(L"\U0001F680",codec,true,0,2,control,&map,&error),L"Big5 maps DBCS units and rejects unrepresentable draft characters");
    cancel = true; check(!MapTextByteRange(raw,codec,true,0,0,control,&map,&error),L"selection mapping cooperatively cancels"); cancel = false;
    UnicodeNotices notices; std::set<std::uint32_t> allowed;
    check(ScanUnicodeNotices("a\xE2\x80\x8B\r\n\xE4\xB8\xAD\xE2\x80\xAE",allowed,control,&notices,&error) && notices.total == 2 && notices.rows[0].offset == 1 && notices.rows[1].line == 2 && notices.rows[1].column == 2,L"Unicode notice scan reports original UTF-8 positions, scalar columns and bidi names");
    check(ParseUnicodeAllowlist(L"U+200B, 202E",&allowed,&error) && ScanUnicodeNotices("\xE2\x80\x8B\xE2\x80\xAE",allowed,control,&notices,&error) && notices.total == 0 && !ParseUnicodeAllowlist(L"D800",&allowed,&error),L"Unicode allowlist suppresses explicitly allowed points and rejects surrogates");
    check(ScanUnicodeNotices("\xE4\xB8\xAD\xE2\x80\x8C\xE2\x80\x8D",{},control,&notices,&error) && notices.total == 0 && !ScanUnicodeNotices("\xFF",{},control,&notices,&error),L"ordinary CJK, ZWNJ and ZWJ stay unflagged; invalid UTF-8 is rejected");
    Leb128Value leb; const std::uint8_t unsigned_example[] = {0xE5,0x8E,0x26},signed_example[] = {0x9B,0xF1,0x59};
    check(DecodeLeb128(unsigned_example,3,false,&leb) && leb.unsigned_value == 624485 && leb.length == 3 && DecodeLeb128(signed_example,3,true,&leb) && leb.signed_value == -624485,L"ULEB128 and SLEB128 decode canonical examples and actual byte lengths");
    std::vector<std::uint8_t> max(10,0xFF); max.back() = 1; check(DecodeLeb128(max.data(),max.size(),false,&leb) && leb.unsigned_value == UINT64_MAX && !DecodeLeb128(max.data(),max.size(),true,&leb),L"64-bit ULEB maximum and signed tenth-byte overflow are distinguished"); max.back() = 0x7F; check(DecodeLeb128(max.data(),max.size(),true,&leb) && leb.signed_value == -1 && !DecodeLeb128(max.data(),max.size(),false,&leb),L"signed tenth-byte extension and unsigned overflow are checked"); max.back() = 0x80; check(!DecodeLeb128(max.data(),max.size(),true,&leb) && !DecodeLeb128(unsigned_example,1,false,&leb) && !DecodeLeb128(nullptr,0,false,&leb),L"LEB continuation overflow, truncated EOF and empty input are rejected");
    DataConversion converted; check(ConvertData("48 69 E4 B8 AD",DataFormat::Hex,DataFormat::Utf8,control,&converted,&error) && converted.output == "Hi\xE4\xB8\xAD",L"Hex to UTF-8 preserves exact decoded bytes");
    check(ConvertData("SGk=",DataFormat::Base64,DataFormat::Hex,control,&converted,&error) && converted.output == "48 69" && ConvertData("Hi",DataFormat::Utf8,DataFormat::Base64,control,&converted,&error) && converted.output == "SGk=",L"standard Base64 round trip has canonical padding");
    for (const auto text : {"A","====","AB==","SGk===","SG=k","SGk!"}) check(!ConvertData(text,DataFormat::Base64,DataFormat::Hex,control,&converted,&error),L"malformed Base64 input rejected");
    check(ConvertData("",DataFormat::Base64,DataFormat::Hex,control,&converted,&error) && converted.output.empty() && !ConvertData("00 FF",DataFormat::Hex,DataFormat::Utf8,control,&converted,&error) && !ConvertData("0x41",DataFormat::Hex,DataFormat::Base64,control,&converted,&error),L"empty conversion is valid; invalid/NUL UTF-8 and address-like Hex are rejected");
    cancel = true; check(!ConvertData("41",DataFormat::Hex,DataFormat::Base64,control,&converted,&error),L"cancelled conversion cannot publish a result"); cancel = false;
    JsonCompareOptions json; StructuredComparison result;
    check(ParseJsonRules(L"ignore /meta/time\nkey /items /id",&json,&error) && CompareStructured(LR"({"meta":{"time":1},"items":[{"id":"b","v":2},{"id":1,"v":3}]})",LR"({"meta":{"time":2},"items":[{"id":1,"v":3},{"id":"b","v":2}]})",StructuredFormat::Json,cancel,&result,&error,{},json) && result.different == 0 && result.ignored == 1,L"JSON explicit keys pair reordered entries while exact ignored pointers skip metadata");
    check(!CompareStructured(LR"({"items":[{"id":1},{"id":1}]})",LR"({"items":[]})",StructuredFormat::Json,cancel,&result,&error,{},json) && !CompareStructured(LR"({"items":[{"v":1}]})",LR"({"items":[]})",StructuredFormat::Json,cancel,&result,&error,{},json),L"duplicate and missing JSON array IDs reject the comparison");
    check(!CompareStructured(LR"({})",LR"({"items":[{"id":1},{"id":1}]})",StructuredFormat::Json,cancel,&result,&error,{},json) && !CompareStructured(LR"({"items":{}})",LR"({"items":[]})",StructuredFormat::Json,cancel,&result,&error,{},json),L"new one-sided keyed arrays still validate duplicates and non-array containers");
    check(CompareStructured(LR"({"items":[{"id":1},{"id":"1"}]})",LR"({"items":[{"id":"1"},{"id":1}]})",StructuredFormat::Json,cancel,&result,&error,{},json) && result.different == 0,L"JSON numeric and string IDs remain distinct typed keys");
    check(!ParseJsonRules(L"ignore /a~2b",&json,&error) && !ParseJsonRules(L"key /items /id\nignore /items/0/v",&json,&error) && ParseJsonRules(L"ignore /a~1b/~0x",&json,&error),L"RFC6901 escapes validated and ambiguous ignores inside keyed arrays rejected");
    WorkspaceEntry source; source.mode = 7; source.left = L"DO_NOT_PERSIST_SOURCE"; source.right = L"SECRET_INPUT"; source.base = L"BASE_PATH"; source.sync_baseline = L"BASELINE_PATH"; source.manual_anchors = L"1,1"; source.json_rules = L"key /items /id"; source.options = 92;
    const auto settings = PresetSettings(source); check(settings.left.empty() && settings.right.empty() && settings.base.empty() && settings.sync_baseline.empty() && settings.manual_anchors.empty() && !settings.sync_deletions,L"comparison preset whitelist removes paths, anchors and synchronization state");
    const auto path = root / L"presets.ini"; std::vector<ComparePreset> presets{{L"JSON keys",source}},loaded;
    check(SaveComparePresets(path,presets,&error) && LoadComparePresets(path,&loaded,&error) && loaded.size() == 1 && loaded[0].name == L"JSON keys" && loaded[0].settings.json_rules == source.json_rules && loaded[0].settings.left.empty(),L"portable named presets round trip without source paths");
    presets.push_back(presets[0]); check(!SaveComparePresets(path,presets,&error) && LoadComparePresets(path,&loaded,&error) && loaded.size() == 1,L"duplicate preset failure retains existing INI");
    std::vector<WorkspaceEntry> entries{source},restored; source.comparison = true; entries[0].comparison = true; check(SaveWorkspaceFile(root / L"workspace.ini",entries,&error) && LoadWorkspaceFile(root / L"workspace.ini",&restored,&error) && restored[0].json_rules == source.json_rules,L"JSON rules persist through ordinary workspace settings");
    return failures;
}

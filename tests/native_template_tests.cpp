#include "../src/core/binary_template.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace simple_editor::core;
int RunTemplateTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; }; const auto path = root / L"template.bin"; const std::vector<std::uint8_t> bytes{0x34,0x12,0xFF,0x7F,0,0,0x80,0x3F,'h','i',0,0}; { std::ofstream out(path,std::ios::binary); out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()); }
    BinaryDocument document; std::wstring error; document.Open(path.wstring(),&error); BinaryTemplateResult result; const std::wstring definition = L"# fixed-layout template\nheader.version u16le 0\nheader.big u16be 0\nheader.signed i8 2\nheader.float f32le 4\nheader.text ascii:4 8\nrecords[].value u8 8 2 d:1\nheader.raw bytes:4 0\nmissing u64le FFFF\n";
    check(InspectBinaryTemplate(document,definition,0,{},&result,&error) && result.fields.size() == 9,L"native field templates expand named structures, arrays and fixed-width types");
    check(result.fields[0].value.find(L"4660") == 0 && result.fields[1].value.find(L"13330") == 0 && result.fields[2].value == L"-1",L"template numeric fields honor endian and signed interpretation");
    check(result.fields[3].value == L"1" && result.fields[4].value == L"hi" && result.fields[4].valid,L"template float and bounded NUL-terminated text fields decode correctly");
    check(result.fields[5].path == L"records[0].value" && result.fields[6].path == L"records[1].value" && result.fields[6].offset == 9,L"array names and explicit strides retain physical offsets");
    check(result.fields[7].value.find(L"34 12 FF 7F") == 0 && !result.fields[8].valid && result.fields[8].value == L"Not enough bytes",L"raw fields and out-of-range data are reported without reading beyond EOF");
    std::vector<std::uint8_t> read; document.Read(0,bytes.size(),&read); check(read == bytes && !document.IsModified(),L"template inspection does not mutate the file or edit history");
    check(InspectBinaryTemplate(document,L"value u8 0",8,{},&result,&error) && result.fields[0].offset == 8 && result.fields[0].value.find(L"104") == 0,L"template bases are physical source offsets");
    check(!InspectBinaryTemplate(document,L"value u8 0 4097",0,{},&result,&error) && !InspectBinaryTemplate(document,L"value f8 0",0,{},&result,&error),L"template count and type bounds reject invalid schemas");
    check(!InspectBinaryTemplate(document,L"value u64le FFFFFFFFFFFFFFFF 2 8",0,{},&result,&error) && !InspectBinaryTemplate(document,L"x command 0",0,{},&result,&error),L"address overflow and executable/unknown template commands are rejected");
    std::atomic_bool cancel{false}; SaveControl control; control.cancelled = &cancel; control.progress = [&](unsigned) { cancel = true; }; const auto old = result.fields.size(); check(!InspectBinaryTemplate(document,definition,0,control,&result,&error) && result.fields.size() == old,L"cancelled template evaluation preserves prior results");
    return failures;
}

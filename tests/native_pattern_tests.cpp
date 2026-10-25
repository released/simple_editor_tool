#include "../src/core/pattern_search.h"
#include <fstream>
#include <iostream>
#include <filesystem>
using namespace simple_editor::core;
int RunPatternTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; }; std::wstring error; BytePattern pattern;
    check(ParseBytePattern(L"AA ?f ??",false,&pattern,&error) && pattern.bytes == std::vector<std::uint8_t>({0xAA,15,0}) && pattern.masks == std::vector<std::uint8_t>({255,15,0}),L"Hex parser retains byte/nibble wildcard masks");
    check(!ParseBytePattern(L"?",false,&pattern,&error) && !ParseBytePattern(L"GG",false,&pattern,&error),L"malformed wildcard pairs are rejected");
    check(ParseBytePattern(L"u32le:305419896",true,&pattern,&error) && pattern.bytes == std::vector<std::uint8_t>({0x78,0x56,0x34,0x12}),L"unsigned little-endian numeric pattern has exact bytes");
    check(ParseBytePattern(L"i16be:-2",true,&pattern,&error) && pattern.bytes == std::vector<std::uint8_t>({0xFF,0xFE}),L"signed big-endian pattern uses two's complement");
    check(ParseBytePattern(L"f32le:1.5",true,&pattern,&error) && pattern.bytes == std::vector<std::uint8_t>({0,0,0xC0,0x3F}),L"floating pattern uses IEEE bits and explicit endian");
    check(ParseBytePattern(L"u64be:18446744073709551615",true,&pattern,&error) && pattern.bytes == std::vector<std::uint8_t>(8,255),L"numeric parser accepts the complete unsigned 64-bit range");
    check(!ParseBytePattern(L"u8:256",true,&pattern,&error) && !ParseBytePattern(L"u8:-1",true,&pattern,&error) && !ParseBytePattern(L"i8:128",true,&pattern,&error) && !ParseBytePattern(L"u32le:12x",true,&pattern,&error),L"numeric overflow, negative unsigned and trailing text are rejected");
    std::vector<std::uint8_t> bytes(2 * 1024 * 1024,0); bytes[1048575] = 0xAA; bytes[1048576] = 0xCF; bytes[1048577] = 0x12; bytes[1800000] = 0xAA; bytes[1800001] = 0x0F;
    const auto path = root / L"patterns.bin"; { std::ofstream out(path,std::ios::binary); out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()); } BinaryDocument document; document.Open(path.wstring(),&error); std::atomic_bool cancel{false}; std::vector<std::uint64_t> offsets; ParseBytePattern(L"AA ?F ??",false,&pattern,&error);
    const auto collect = [&](std::uint64_t at) { offsets.push_back(at); return true; };
    check(ScanBytePattern(document,pattern,0,document.Size(),cancel,collect,{},&error) && offsets == std::vector<std::uint64_t>({1048575,1800000}),L"streaming wildcard search spans chunk boundaries without duplicate matches"); offsets.clear();
    check(ScanBytePattern(document,pattern,1500000,1900000,cancel,collect,{},&error) && offsets == std::vector<std::uint64_t>({1800000}),L"byte search confines every match to the requested range"); offsets.clear(); cancel = true;
    check(ScanBytePattern(document,pattern,0,document.Size(),cancel,collect,{},&error) && offsets.empty(),L"cancelled pattern search does not emit results"); cancel = false;
    ParseBytePattern(L"?? ??",false,&pattern,&error); offsets.clear(); check(ScanBytePattern(document,pattern,0,4,cancel,collect,{},&error) && offsets == std::vector<std::uint64_t>({0,1,2}),L"all-wildcard patterns support overlapping streaming matches");
    return failures;
}

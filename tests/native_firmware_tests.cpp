#include "../src/core/firmware_image.h"
#include <fstream>
#include <iostream>
using namespace simple_editor::core;
namespace {
void Write(const std::filesystem::path& path,const std::string& bytes) { std::ofstream out(path,std::ios::binary); out.write(bytes.data(),bytes.size()); }
std::string Read(const std::filesystem::path& path) { std::ifstream in(path,std::ios::binary); return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()}; }
}
int RunFirmwareTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
    std::wstring error; SaveControl control; FirmwareImage image;
    const auto path = root / L"records.hex";
    const auto parse = [&](const std::string& text,FirmwareFormat format = FirmwareFormat::IntelHex) { Write(path,text); BinaryDocument source; return source.Open(path.wstring(),&error) && ParseFirmware(source,format,control,&image,&error); };
    check(parse(":020000040001F9\n:0400100001020304E2\n:00000001FF\n") && image.Begin() == 0x10010 && image.data_bytes == 4 && image.segments[0].bytes == std::vector<std::uint8_t>({1,2,3,4}),L"Intel HEX validates checksum and extended linear addresses");
    check(!parse(":0400100001020304E3\n:00000001FF\n"),L"Intel HEX rejects a wrong record checksum");
    check(!parse(":04001000010203E3\n:00000001FF\n"),L"Intel HEX rejects a truncated count");
    check(!parse(":00000001FF\n:00000001FF\n"),L"records after EOF are rejected");
    check(!parse(":0100000041BE\n"),L"missing Intel EOF is rejected");
    check(!parse(":0100000041BE\n:0100000041BE\n:00000001FF\n"),L"overlapping firmware addresses are rejected even when bytes match");
    check(parse(":020000021234B6\n:01000200AA53\n:00000001FF\n") && image.Begin() == 0x12342,L"extended segment address is shifted by four bits");
    check(parse(":020000021000EC\n:02FFFF001122CD\n:00000001FF\n") && image.segments.size() == 2 && image.segments[0].address == 0x10000 && image.segments[0].bytes[0] == 0x22 && image.segments[1].address == 0x1FFFF,L"segment-offset wrapping preserves each effective address");
    check(parse(":040000050801234586\n:00000001FF\n") && image.has_entry && image.entry == 0x08012345,L"Intel start address is represented separately from data");
    check(parse("S00600004844521B\nS110000048656C6C6F2C20576F726C640A9D\nS5030001FB\nS9030000FC\n",FirmwareFormat::SRecord) && image.data_bytes == 13 && image.entry == 0,L"S-record header/data/count/termination validate the specification example");
    check(!parse("S104000041BA\nS5030002FA\nS9030000FC\n",FirmwareFormat::SRecord),L"S-record declared data count is checked");
    check(!parse("S104000041BB\nS9030000FC\n",FirmwareFormat::SRecord),L"S-record checksum errors are rejected");
    check(parse(":0100000041BE\n:0100040042B9\n:00000001FF\n") && image.segments.size() == 2 && image.End() == 5,L"sparse segments remain distinct in the address map");
    BinaryDocument records; records.Open(path.wstring(),&error); const auto output = root / L"image.bin";
    check(FirmwareToBinary(records,image,0,5,0xFF,output,control,&error) && Read(output) == std::string("A\xFF\xFF\xFF" "B",5),L"BIN export fills sparse gaps with the selected byte");
    check(FirmwareToBinary(records,image,1,3,0,root / L"range.bin",control,&error) && Read(root / L"range.bin") == std::string(3,'\0'),L"firmware range export honors an explicit start and length");
    check(!FirmwareToBinary(records,image,UINT32_MAX,2,0,output,control,&error),L"firmware output rejects a wrapping 32-bit range");
    check(!FirmwareToBinary(records,image,0,5,0,path,control,&error),L"firmware conversion cannot overwrite its record source");
    const auto bin = root / L"original.bin"; std::string bytes; for (unsigned i = 0; i < 300; ++i) bytes.push_back(static_cast<char>(i)); Write(bin,bytes); BinaryDocument source; source.Open(bin.wstring(),&error);
    for (const auto format : {FirmwareFormat::IntelHex,FirmwareFormat::SRecord}) {
        const auto encoded = root / (format == FirmwareFormat::IntelHex ? L"roundtrip.hex" : L"roundtrip.srec"); BinaryDocument text;
        check(BinaryToFirmware(source,0x1234FFF0,format,encoded,control,&error) && text.Open(encoded.wstring(),&error) && ParseFirmware(text,format,control,&image,&error) && image.Begin() == 0x1234FFF0 && image.data_bytes == 300 && FirmwareToBinary(text,image,image.Begin(),300,0,output,control,&error) && Read(output) == bytes,format == FirmwareFormat::IntelHex ? L"BIN/Intel HEX round trip crosses a 64-KiB boundary exactly" : L"BIN/S3 S-record round trip preserves 32-bit addresses and bytes");
    }
    std::atomic_bool cancelled{true}; control.cancelled = &cancelled; check(!BinaryToFirmware(source,0,FirmwareFormat::IntelHex,output,control,&error) && Read(output) == bytes,L"cancelled firmware conversion retains the previous output");
    return failures;
}

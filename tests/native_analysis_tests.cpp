#include "../src/core/binary_analysis.h"
#include <fstream>
#include <iostream>
#include <cmath>
using namespace simple_editor::core;
namespace {
void Write(const std::filesystem::path& path,const std::string& bytes) { std::ofstream out(path,std::ios::binary); out.write(bytes.data(),bytes.size()); }
std::string Read(const std::filesystem::path& path) { std::ifstream in(path,std::ios::binary); return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()}; }
std::string Bytes(const BinaryDocument& document) { std::vector<std::uint8_t> bytes; document.Read(0,static_cast<std::size_t>(document.Size()),&bytes); return {bytes.begin(),bytes.end()}; }
}
int RunAnalysisTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
    const auto input = root / L"analysis.bin", output = root / L"extract.bin"; Write(input,"123456789"); BinaryDocument document; std::wstring error; document.Open(input.wstring(),&error); SaveControl control; BinaryAnalysis analysis;
    check(AnalyzeBinary(document,0,9,control,&analysis,&error) && analysis.crc32 == 0xCBF43926 && analysis.sha256 == L"15E2B0D3C33891EBB0F1EF609EC419420C20E320CE94C65FBC8C3312448EB225",L"CRC32/SHA-256 match the published numeric test vector");
    check(analysis.bytes == 9 && analysis.histogram['1'] == 1 && std::abs(analysis.entropy - std::log2(9.0)) < 1e-10,L"analysis range histogram and Shannon entropy are exact");
    check(AnalyzeBinary(document,9,0,control,&analysis,&error) && analysis.crc32 == 0 && analysis.sha256 == L"E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855" && analysis.entropy == 0,L"empty range has standard empty hashes and zero entropy");
    std::vector<InspectorValue> values; check(InspectNumbers(document,0,true,&values,&error) && values[2].value.find(L"12849") == 0,L"little-endian inspector decodes a two-byte integer");
    check(InspectNumbers(document,0,false,&values,&error) && values[2].value.find(L"12594") == 0,L"big-endian inspector reverses interpretation without editing bytes");
    check(InspectNumbers(document,9,true,&values,&error) && !values[0].valid && !values.back().valid,L"inspector reports insufficient bytes at EOF");
    const auto floating = root / L"floats.bin"; Write(floating,std::string("\0\0\x80\x3f\xFF\xFF\xFF\xFF",8)); BinaryDocument floats; floats.Open(floating.wstring(),&error);
    check(InspectNumbers(floats,0,true,&values,&error) && values[6].value == L"1" && InspectNumbers(floats,4,true,&values,&error) && values[5].value == L"-1",L"inspector handles IEEE float and sign extension");
    check(ExtractBytes(document,2,4,output,control,&error) && Read(output) == "3456",L"range extraction writes exactly the selected bytes");
    check(!ExtractBytes(document,8,2,output,control,&error) && Read(output) == "3456",L"out-of-bounds extraction preserves the previous output");
    const auto alias = root / L"analysis-alias.bin"; CreateHardLinkW(alias.c_str(),input.c_str(),nullptr);
    check(!ExtractBytes(document,0,9,alias,control,&error) && Read(input) == "123456789",L"range tools reject outputs aliasing their source");
    check(ExtractBytes(document,0,2,root / L"array.h",control,&error,true) && Read(root / L"array.h").find("0x31, 0x32,") != std::string::npos,L"C array export encodes bytes and original data size");
    check(TransformBytes(document,1,4,RangeTransform::Fill,{0xAB,0xCD},control,&error) && Bytes(document) == std::string("1\xAB\xCD\xAB\xCD" "6789",9),L"repeating fill updates only the selected range");
    std::uint64_t at = 0; check(document.Undo(&at) && Bytes(document) == "123456789" && document.Redo(&at) && document.Undo(&at),L"range transformations are one complete Undo/Redo action");
    check(TransformBytes(document,0,8,RangeTransform::Swap4,{},control,&error) && Bytes(document) == "432187659",L"word swapping preserves group boundaries"); document.Undo(&at);
    check(!TransformBytes(document,0,7,RangeTransform::Swap4,{},control,&error) && Bytes(document) == "123456789",L"misaligned swap is rejected without editing");
    check(TransformBytes(document,1,3,RangeTransform::Reverse,{},control,&error) && Bytes(document) == "143256789",L"reverse transforms only the selected bytes"); document.Undo(&at);
    std::atomic_bool cancel{true}; control.cancelled = &cancel;
    check(!TransformBytes(document,0,9,RangeTransform::Invert,{},control,&error) && !AnalyzeBinary(document,0,9,control,&analysis,&error) && Bytes(document) == "123456789",L"cancelled analysis and transformation retain the document"); cancel = false;
    std::vector<std::filesystem::path> completed; const auto prefix = root / L"parts";
    check(SplitBinaryFile(document,4,prefix,control,&completed,&error) && completed.size() == 3 && Read(completed[0]) == "1234" && Read(completed[2]) == "9",L"file splitting preserves order and short final part");
    check(JoinBinaryFiles(completed,root / L"joined.bin",control,&error) && Read(root / L"joined.bin") == "123456789",L"joining split parts reconstructs the original bytes");
    check(!SplitBinaryFile(document,4,prefix,control,&completed,&error) && Read(root / L"parts.part1.bin") == "1234",L"splitting never overwrites an existing part");
    check(!JoinBinaryFiles({input,output},input,control,&error) && Read(input) == "123456789",L"joining rejects every input as an output");
    Write(output,"keep"); control.progress = [&](unsigned p) { if (p == 99) Write(output,"changed"); };
    check(!ExtractBytes(document,0,9,output,control,&error) && Read(output) == "changed",L"range extraction rejects a changed destination at commit"); control.progress = {};
    std::string distribution; for (unsigned i = 0; i < 256; ++i) distribution.push_back(static_cast<char>(i)); const auto distributed = root / L"uniform.bin"; Write(distributed,distribution); BinaryDocument uniform; uniform.Open(distributed.wstring(),&error);
    check(AnalyzeBinary(uniform,0,256,control,&analysis,&error) && std::abs(analysis.entropy - 8.0) < 1e-10 && analysis.entropy_blocks.size() <= 512,L"uniform byte distribution has eight bits per byte and a bounded block series");
    check(!ExtractBytes(document,0,9,output,control,&error,false,true) && Read(output) == "changed",L"new-output guard rejects files appearing before output creation");
    return failures;
}

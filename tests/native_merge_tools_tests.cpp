#include "../src/core/binary_merge.h"
#include "../src/core/text_output.h"
#include <fstream>
#include <iostream>
using namespace simple_editor::core;
namespace {
void Write(const std::filesystem::path& path,const std::string& data) { std::ofstream out(path,std::ios::binary); out.write(data.data(),data.size()); }
std::string Read(const std::filesystem::path& path) { std::ifstream in(path,std::ios::binary); return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()}; }
}
int RunMergeToolsTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
    const auto left_path = root / L"baseline.bin", right_path = root / L"donor.bin", out = root / L"binary-merge.bin"; Write(left_path,"abcdef"); Write(right_path,"abXYcdZf!"); BinaryDocument left,right; std::wstring error; left.Open(left_path.wstring(),&error); right.Open(right_path.wstring(),&error); SaveControl control;
    const std::vector<Difference> differences{{kMissingOffset,2,-1,'X'},{kMissingOffset,3,-1,'Y'},{4,6,'e','Z'},{kMissingOffset,8,-1,'!'}}; std::vector<BinaryPatch> patches;
    check(BuildBinaryPatches(differences,{0,1,2,3},true,6,9,&patches,&error) && patches.size() == 3 && patches[0].offset == 2 && patches[0].remove == 0 && patches[0].length == 2,L"binary patch plan maps aligned insertions and coalesces contiguous selected bytes");
    check(MergeBinaryRanges(left,right,patches,out,control,&error) && Read(out) == "abXYcdZf!",L"streaming range merge produces exact inserted/changed/EOF bytes");
    check(BuildBinaryPatches(differences,{0},true,6,9,&patches,&error) && MergeBinaryRanges(left,right,patches,out,control,&error) && Read(out) == "abXcdef",L"selecting part of an insertion preserves unselected baseline bytes");
    check(BuildBinaryPatches(differences,{0,1,2,3},false,6,9,&patches,&error) && MergeBinaryRanges(right,left,patches,out,control,&error) && Read(out) == "abcdef",L"reverse range merge handles donor deletions and shifted offsets");
    check(!BuildBinaryPatches(differences,{4},true,6,9,&patches,&error),L"invalid selected difference index is rejected");
    check(!BuildBinaryPatches({{UINT64_MAX - 1,0,1,1}},{0},true,6,9,&patches,&error),L"malformed difference addresses cannot overflow signed alignment arithmetic");
    check(!MergeBinaryRanges(left,right,{{7,0,0,1}},out,control,&error) && Read(out) == "abcdef",L"out-of-range merge retains the old output");
    check(!MergeBinaryRanges(left,right,{},left_path,control,&error),L"binary merge cannot overwrite either input"); const auto alias = root / L"merge-alias.bin"; CreateHardLinkW(alias.c_str(),right_path.c_str(),nullptr);
    check(!MergeBinaryRanges(left,right,{},alias,control,&error),L"binary merge rejects source hard-link aliases");
    std::atomic_bool cancelled{true}; control.cancelled = &cancelled;
    check(!MergeBinaryRanges(left,right,{{0,0,0,1}},out,control,&error) && Read(out) == "abcdef",L"cancelled binary merge preserves the old output"); cancelled = false;
    control.progress = [&](unsigned p) { if (p == 99) Write(out,"external"); };
    check(!MergeBinaryRanges(left,right,{},out,control,&error) && Read(out) == "external",L"merge destination is revalidated at the commit boundary"); control.progress = {};
    TextOutputOptions options; options.codec = {TextEncoding::Utf16Be,1201,true}; options.eol = OutputEol::Lf; std::vector<std::uint8_t> bytes;
    const std::wstring mixed = L"a\r\nb\nc\r\x4E2D"; std::wstring decoded; TextCodec codec;
    check(ConvertTextOutput(mixed,options,&bytes,&error) && bytes[0] == 0xFE && bytes[1] == 0xFF && DecodeText(bytes,TextEncoding::Auto,&decoded,&codec,&error) && decoded == L"a\nb\nc\n\x4E2D",L"explicit UTF-16 BE output converts BOM and all newline styles");
    options.codec = {TextEncoding::Utf8,CP_UTF8,false}; options.eol = OutputEol::Preserve;
    check(ConvertTextOutput(mixed,options,&bytes,&error) && DecodeText(bytes,TextEncoding::Auto,&decoded,&codec,&error) && decoded == mixed,L"preserve mode retains mixed source endings and final unterminated text");
    options.eol = OutputEol::CrLf; check(ConvertTextOutput(L"a\nb\rc\r\n",options,&bytes,&error) && std::string(bytes.begin(),bytes.end()) == "a\r\nb\r\nc\r\n",L"CRLF conversion handles LF, CR and existing CRLF exactly once");
    options.eol = OutputEol::Cr; check(ConvertTextOutput(L"a\nb",options,&bytes,&error) && std::string(bytes.begin(),bytes.end()) == "a\rb",L"CR conversion preserves a missing final newline");
    options.codec = {TextEncoding::Big5,950,true}; check(!ConvertTextOutput(mixed,options,&bytes,&error),L"legacy encodings cannot claim an unsupported BOM");
    options.codec.bom = false; check(!ConvertTextOutput(L"\U0001F680",options,&bytes,&error),L"lossy legacy output conversion is rejected");
    options.codec = {TextEncoding::Utf8,CP_UTF8,true}; options.eol = OutputEol::Lf; const std::vector<ProtectedInput> inputs{{left.Path(),left.SourceIdentity()}};
    check(ExportTextOutput(L"converted",options,inputs,root / L"converted.txt",control,&error) && Read(root / L"converted.txt") == std::string("\xEF\xBB\xBF" "converted"),L"guarded conversion writes the requested encoded output copy");
    check(!ExportTextOutput(L"converted",options,inputs,left_path,control,&error) && Read(left_path) == "abcdef",L"output conversion keeps its original input intact");
    return failures;
}

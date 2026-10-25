#include "../src/core/binary_report.h"
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace simple_editor::core;
namespace {
void Write(const std::filesystem::path& path, const std::vector<unsigned char>& bytes) { std::ofstream stream(path,std::ios::binary); stream.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()); }
std::string Read(const std::filesystem::path& path) { std::ifstream stream(path,std::ios::binary); return {std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()}; }
}
int RunBinaryToolsTests(const std::filesystem::path& root) {
    int failed = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failed; };
    const auto a = root / L"left&source.bin", b = root / L"right.bin", output = root / L"differences.csv";
    Write(a,{0,1,2,3}); Write(b,{0,255,2,3,170}); BinaryDocument left,right; std::wstring error; left.Open(a.wstring(),&error); right.Open(b.wstring(),&error);
    std::atomic_bool cancel{false}; SaveControl control; control.cancelled = &cancel; BinaryReportStats stats;
    check(ExportBinaryReport(left,right,false,output,BinaryReportFormat::Csv,control,&stats,&error) && stats.changed == 1 && stats.added == 1 && stats.removed == 0 && stats.ranges == 2,L"CSV report has exact byte counts and contiguous ranges");
    const auto csv = Read(output);
    check(csv.find("changed,0x0000000000000001,01,0x0000000000000001,FF") != std::string::npos && csv.find("added,,,0x0000000000000004,AA") != std::string::npos,L"CSV preserves missing offsets and exact hexadecimal addresses/data");
    check(ExportBinaryReport(right,left,false,root / L"reverse.tsv",BinaryReportFormat::Tsv,control,&stats,&error) && stats.removed == 1 && Read(root / L"reverse.tsv").find("removed\t0x0000000000000004\tAA\t\t") != std::string::npos,L"TSV reverse comparison distinguishes removals");
    check(ExportBinaryReport(left,right,false,root / L"report.html",BinaryReportFormat::Html,control,&stats,&error) && Read(root / L"report.html").find("left&amp;source.bin") != std::string::npos,L"HTML report escapes paths and contains the comparison table");
    check(!ExportBinaryReport(left,right,false,a,BinaryReportFormat::Csv,control,&stats,&error) && Read(a) == std::string("\0\1\2\3",4),L"reports cannot overwrite a comparison source");
    const auto alias = root / L"alias.bin"; const bool linked = CreateHardLinkW(alias.c_str(),a.c_str(),nullptr) != FALSE;
    check(linked && !ExportBinaryReport(left,right,false,alias,BinaryReportFormat::Csv,control,&stats,&error),L"report source protection also rejects hard-link aliases");
    Write(output,{'k','e','e','p'}); cancel = true;
    check(!ExportBinaryReport(left,right,false,output,BinaryReportFormat::Csv,control,&stats,&error) && Read(output) == "keep",L"cancelled report retains the existing output"); cancel = false;
    control.progress = [&](unsigned p) { if (p == 99) Write(output,{'e','x','t','e','r','n','a','l'}); };
    check(!ExportBinaryReport(left,right,false,output,BinaryReportFormat::Csv,control,&stats,&error) && Read(output) == "external",L"output changes at the commit boundary are retained"); control.progress = {};
    check(!ExportBinaryReport(left,right,false,root,BinaryReportFormat::Csv,control,&stats,&error),L"directory output is rejected");
    bool clean = true; for (const auto& file : std::filesystem::directory_iterator(root)) if (file.path().filename().wstring().find(L".set-output-") == 0) clean = false;
    check(clean,L"success/failure/cancellation remove only owned output temporaries");
    const auto large_a = root / L"large-a.bin", large_b = root / L"large-b.bin";
    Write(large_a,std::vector<unsigned char>(1000005,'A')); Write(large_b,std::vector<unsigned char>(1000005,'B'));
    BinaryDocument big_left,big_right; big_left.Open(large_a.wstring(),&error); big_right.Open(large_b.wstring(),&error);
    check(ExportBinaryReport(big_left,big_right,false,root / L"full.csv",BinaryReportFormat::Csv,control,&stats,&error) && stats.changed == 1000005 && stats.ranges == 1,L"streaming report is not truncated at the one-million-row UI limit");
    std::ifstream full(root / L"full.csv",std::ios::binary); std::uint64_t lines = 0; std::string line; while (std::getline(full,line)) ++lines;
    check(lines == 1000006,L"full CSV contains the header and every reported difference");
    return failed;
}

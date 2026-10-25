#include "../src/core/command_line.h"
#include <fstream>
#include <iostream>
#include <filesystem>
using namespace simple_editor::core;
namespace {
void Write(const std::filesystem::path& path,const std::string& value) { std::ofstream out(path,std::ios::binary); out << value; }
std::string Read(const std::filesystem::path& path) { std::ifstream in(path,std::ios::binary); return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()}; }
}
int RunCliTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; }; const auto a = root / L"cli-left.bin",b = root / L"cli-right.bin",report = root / L"cli-report.csv"; Write(a,"abc"); Write(b,"abc");
    const auto args = [&](const std::wstring& mode = L"binary") { return std::vector<std::wstring>{L"--compare",L"--left",a.wstring(),L"--right",b.wstring(),L"--mode",mode}; };
    check(CompareCommandLine({L"--help"}).exit_code == 0 && CompareCommandLine({L"--help"}).help,L"headless help has a successful exit code");
    check(CompareCommandLine(args()).exit_code == 0,L"equal binary inputs return exit code zero"); Write(b,"a!c"); auto result = CompareCommandLine(args()); check(result.exit_code == 1 && result.differences == 1,L"different binary inputs return one with a full differing-byte count");
    auto input = args(); input.insert(input.end(),{L"--report",report.wstring()}); result = CompareCommandLine(input); check(result.exit_code == 1 && Read(report).find("changed") != std::string::npos && Read(a) == "abc" && Read(b) == "a!c",L"CLI binary reports retain both source files");
    Write(report,"existing report"); check(CompareCommandLine(input).exit_code == 2 && Read(report) == "existing report",L"existing reports require an explicit overwrite flag"); input.push_back(L"--overwrite"); check(CompareCommandLine(input).exit_code == 1 && Read(report) != "existing report",L"explicit report overwrite uses the guarded streaming writer");
    input = args(); input.insert(input.end(),{L"--report",a.wstring(),L"--overwrite"}); check(CompareCommandLine(input).exit_code == 2 && Read(a) == "abc",L"CLI report output cannot replace an input even with overwrite");
    input = args(); input.insert(input.end(),{L"--region",L"0,1,0,1"}); check(CompareCommandLine(input).exit_code == 0,L"CLI regional comparison uses the same source offsets as the GUI");
    input = args(L"text"); input.insert(input.end(),{L"--report",(root / L"cli-text.patch").wstring(),L"--format",L"patch"}); check(CompareCommandLine(input).exit_code == 1 && Read(root / L"cli-text.patch").find("@@") != std::string::npos,L"CLI text comparison produces validated unified patches without a GUI");
    const auto base = root / L"cli-base.txt"; Write(base,"abc"); input = args(L"threeway"); input.insert(input.end(),{L"--base",base.wstring(),L"--report",(root / L"cli-threeway.txt").wstring()}); check(CompareCommandLine(input).exit_code == 1 && Read(root / L"cli-threeway.txt").find("Conflicts:") != std::string::npos,L"three-way CLI summaries include the base and conflict count");
    check(CompareCommandLine(args(L"threeway")).exit_code == 2 && CompareCommandLine({L"--compare",L"--unknown"}).exit_code == 2,L"missing base/unknown arguments return errors");
    std::atomic_bool cancel{true}; SaveControl control; control.cancelled = &cancel; check(CompareCommandLine(args(),control).exit_code == 3 && Read(a) == "abc",L"cancelled headless comparisons return three without writes");
    const auto left = root / L"cli-folder-left",right = root / L"cli-folder-right"; std::filesystem::create_directories(left); std::filesystem::create_directories(right); Write(left / L"file.txt","same"); Write(right / L"file.txt","same"); input = {L"--compare",L"--mode",L"folders",L"--left",left.wstring(),L"--right",right.wstring(),L"--report",(root / L"cli-folders.csv").wstring()}; check(CompareCommandLine(input).exit_code == 0 && Read(root / L"cli-folders.csv").find("file.txt") != std::string::npos,L"folder CLI reports include compared entries");
    input[input.size() - 1] = (left / L"report.csv").wstring(); check(CompareCommandLine(input).exit_code == 2 && !std::filesystem::exists(left / L"report.csv"),L"folder report destinations must remain outside both input roots");
    return failures;
}

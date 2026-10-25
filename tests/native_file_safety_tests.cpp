#include "../src/core/binary_document.h"
#include "../src/core/diff_engine.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace simple_editor::core;
namespace {
void Write(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}
std::vector<std::uint8_t> ReadDisk(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
void ChangeWriteTime(const std::filesystem::path& path) {
    const HANDLE handle = CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    FILETIME time{}; GetSystemTimeAsFileTime(&time);
    ULARGE_INTEGER value{}; value.HighPart = time.dwHighDateTime; value.LowPart = time.dwLowDateTime; value.QuadPart += 100000000;
    time.dwHighDateTime = value.HighPart; time.dwLowDateTime = value.LowPart;
    SetFileTime(handle, nullptr, nullptr, &time); CloseHandle(handle);
}
std::size_t TempFiles(const std::filesystem::path& root) {
    std::size_t count = 0;
    for (const auto& item : std::filesystem::directory_iterator(root)) if (item.path().filename().wstring().find(L".tmp") != std::wstring::npos) ++count;
    return count;
}
}

int RunFileSafetyTests(const std::filesystem::path& root) {
    int failures = 0;
    const auto check = [&](bool condition, const wchar_t* name) {
        if (!condition) { ++failures; std::wcerr << L"FAIL: " << name << L'\n'; }
        else std::wcout << L"PASS: " << name << L'\n';
    };
    const auto source = root / L"guard-source.bin";
    const auto replacement = root / L"replacement.bin";
    const auto backup = root / L"recovered.bin";
    std::wstring error; std::vector<std::uint8_t> bytes;
    BinaryDocument document;
    Write(source, {'A','B','C'}); check(document.Open(source.wstring(), &error), L"open guarded source");
    check(document.SetByte(1, 'X'), L"edit guarded source");
    Write(replacement, {'N','E','W'});
    const bool replaced = ReplaceFileW(source.c_str(), replacement.c_str(), nullptr, 0, nullptr, nullptr) != FALSE;
    if (!replaced) std::wcout << L"Replacement error: " << GetLastError() << L'\n';
    check(replaced, L"external atomic replacement fixture");
    check(!document.ValidateSource(true, &error) && document.ValidateSource(false), L"path replacement detected while original handle stays valid");
    check(!document.Save(L"", &error) && document.IsModified() && document.IsOpen(), L"save refuses to overwrite external replacement");
    check(ReadDisk(source) == std::vector<std::uint8_t>({'N','E','W'}), L"external replacement remains intact");
    check(document.Read(0, 3, &bytes) && bytes == std::vector<std::uint8_t>({'A','X','C'}), L"failed save retains editor bytes");
    check(document.Save(backup.wstring(), &error) && ReadDisk(backup) == std::vector<std::uint8_t>({'A','X','C'}), L"Save As recovers edits from intact original handle");

    Write(source, {'A','B','C'}); document.Open(source.wstring(), &error); document.SetByte(0, 'Q');
    Write(source, {'Z','Z','Z'}); ChangeWriteTime(source);
    const auto temporaries = TempFiles(root);
    check(!document.Read(0, 3, &bytes) && bytes.empty(), L"same-size external edits reject stale reads");
    check(!document.Save(L"", &error) && document.IsModified() && ReadDisk(source) == std::vector<std::uint8_t>({'Z','Z','Z'}), L"same-size external edits block stale save");
    check(!document.Snapshot(&error) && TempFiles(root) == temporaries, L"stale snapshots fail without temporary-file leaks");

    document.Close(); Write(source, {'A','B','C'}); document.Open(source.wstring(), &error); document.SetByte(0, 'Q');
    check(DeleteFileW(source.c_str()) != FALSE && !document.Save(L"", &error), L"save does not silently recreate an externally deleted source");
    check(!std::filesystem::exists(source) && document.IsModified(), L"deleted source and edit history preserved after conflict");
    check(document.Save((root / L"deleted-recovery.bin").wstring(), &error), L"deleted-source edits can be recovered by Save As");

    const auto locked = root / L"locked.bin";
    Write(source, {'A','B','C'}); document.Open(source.wstring(), &error); document.SetByte(0, 'Q'); Write(locked, {'L'});
    const HANDLE lock = CreateFileW(locked.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    const auto before = TempFiles(root);
    check(lock != INVALID_HANDLE_VALUE && !document.Save(locked.wstring(), &error), L"destination lock causes a non-destructive save failure");
    check(ReadDisk(locked) == std::vector<std::uint8_t>({'L'}) && document.IsModified() && document.Path() == source.wstring(), L"failed Save As preserves destination and document identity");
    check(TempFiles(root) == before && document.Read(0, 3, &bytes) && bytes[0] == 'Q', L"failed commit cleans temporary file and retains source handle");
    std::uint64_t at = 0; check(document.Undo(&at) && document.Redo(&at), L"undo and redo survive a failed save");
    CloseHandle(lock);
    check(document.Save(locked.wstring(), &error) && document.ValidateSource(true) && document.Read(0, 3, &bytes) && bytes[0] == 'Q', L"retry commits and adopts a readable validated handle");
    check(TempFiles(root) == before, L"successful retry removes transaction backup files");

    const auto left_path = root / L"compare-left.bin", right_path = root / L"compare-right.bin";
    for (bool aligned : {false, true}) {
        const std::vector<std::uint8_t> content(3 * 1024 * 1024, 'A'); Write(left_path, content); Write(right_path, content);
        std::atomic_bool cancelled(false); bool truncated = false;
        const bool ok = CompareFiles(left_path.wstring(), right_path.wstring(), aligned, cancelled,
            [](const DifferenceBatch&) { return true; }, [&](unsigned int percent) {
                if (percent > 0 && !truncated) { truncated = true; Write(right_path, {}); }
            }, &error);
        check(truncated && !ok && !error.empty(), aligned ? L"aligned comparison exits on mid-run truncation" : L"equal-address comparison exits on mid-run truncation");
    }
    Write(left_path, {}); Write(right_path, {}); std::atomic_bool cancelled(false); unsigned int progress = 0;
    check(CompareFiles(left_path.wstring(), right_path.wstring(), true, cancelled, [](const DifferenceBatch&) { return true; }, [&](auto value) { progress = value; }, &error) && progress == 100 && error.empty(), L"empty comparison finishes at 100 percent and clears old errors");
    Write(left_path, std::vector<std::uint8_t>(100000, 'A')); Write(right_path, std::vector<std::uint8_t>(100000, 'B'));
    bool bounded = true; std::size_t count = 0;
    check(CompareFiles(left_path.wstring(), right_path.wstring(), true, cancelled, [&](const DifferenceBatch& batch) { bounded = bounded && batch.size() <= 4096; count += batch.size(); return true; }, [](auto) {}, &error) && bounded && count == 100000, L"aligned result batches are bounded to 4096 rows");
    count = 0;
    check(CompareFiles(left_path.wstring(), right_path.wstring(), true, cancelled, [&](const DifferenceBatch& batch) { count += batch.size(); cancelled = true; return true; }, [](auto) {}, &error) && count == 4096, L"cooperative cancellation stops before a second result batch");

    BinaryDocument left, right; Write(left_path, {'A'}); Write(right_path, {'A'});
    left.Open(left_path.wstring(), &error); right.Open(right_path.wstring(), &error); Write(replacement, {'B'});
    check(ReplaceFileW(right_path.c_str(), replacement.c_str(), nullptr, 0, nullptr, nullptr) != FALSE, L"replace an opened comparison path"); cancelled = false; count = 0;
    check(CompareDocuments(left, right, false, cancelled, [&](const auto& batch) { count += batch.size(); return true; }, [](auto) {}, &error) && count == 0, L"opened-handle comparison stays on the displayed file version");
    if (!failures) std::wcout << L"PASS: native file safety tests\n";
    return failures;
}

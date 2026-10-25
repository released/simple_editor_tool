#include "../src/core/binary_document.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>

using namespace simple_editor::core;
namespace {
void Write(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream file(path, std::ios::binary); file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}
std::vector<std::uint8_t> Read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary); return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
bool NoTemporaryFiles(const std::filesystem::path& root) {
    for (const auto& file : std::filesystem::directory_iterator(root)) if (file.path().filename().wstring().find(L".tmp") != std::wstring::npos) return false;
    return true;
}
}

int RunAsyncSaveTests(const std::filesystem::path& root) {
    int failures = 0;
    const auto check = [&](bool ok, const wchar_t* name) { if (ok) std::wcout << L"PASS: " << name << L'\n'; else { ++failures; std::wcerr << L"FAIL: " << name << L'\n'; } };
    const auto source = root / L"async-source.bin", target = root / L"async-target.bin";
    const std::vector<std::uint8_t> original(4 * 1024 * 1024, 'A');
    std::wstring error; BinaryDocument document; std::atomic_bool cancel(false); SaveControl control; control.cancelled = &cancel;
    for (const unsigned int stop : {0U, 24U, 99U}) {
        Write(source, original); Write(target, {'O','L','D'}); document.Open(source.wstring(), &error); document.SetByte(0, 'Z'); cancel = false;
        unsigned int last = 0; bool monotonic = true;
        control.progress = [&](unsigned int value) { monotonic = monotonic && value >= last; last = value; if (value >= stop) cancel = true; };
        check(!document.Save(target.wstring(), &error, control) && cancel && !error.empty(), L"save cancellation is honoured before replacement");
        check(Read(target) == std::vector<std::uint8_t>({'O','L','D'}) && document.IsModified() && document.Path() == source.wstring(), L"cancelled save preserves destination and editor identity");
        check(monotonic && NoTemporaryFiles(root), L"cancelled save reports monotonic progress and cleans temporary files");
        std::uint64_t at = 0; check(document.Undo(&at) && document.Redo(&at), L"cancelled save preserves Undo/Redo");
    }
    cancel = false; control.progress = [&](unsigned int value) { if (value == 100) cancel = true; };
    check(document.Save(target.wstring(), &error, control) && cancel && !document.IsModified() && document.Path() == target.wstring(), L"post-commit cancellation still reports a successful save");
    auto written = Read(target); check(written.size() == original.size() && written[0] == 'Z' && NoTemporaryFiles(root), L"successful save writes complete bytes and removes backup");

    document.Open(source.wstring(), &error); document.SetByte(0, 'Q'); cancel = false; Write(target, {'O','L','D'});
    control.progress = [&](unsigned int value) { if (value == 99) Write(target, {'E','X','T','E','R','N','A','L'}); };
    check(!document.Save(target.wstring(), &error, control) && Read(target) == std::vector<std::uint8_t>({'E','X','T','E','R','N','A','L'}) && document.IsModified(), L"destination change immediately before commit aborts safely");

    auto snapshot = document.Snapshot(&error); check(snapshot != nullptr, L"create background save snapshot");
    if (snapshot) {
        std::promise<void> entered, release; auto released = release.get_future(); auto ready = entered.get_future();
        control.progress = [&](unsigned int value) { if (value == 0) { entered.set_value(); released.wait(); } };
        auto saved = std::async(std::launch::async, [snapshot = std::move(snapshot), &error, &control, target]() mutable { return snapshot->Save(target.wstring(), &error, control); });
        ready.wait(); std::vector<std::uint8_t> bytes;
        check(document.Read(0, 3, &bytes) && bytes[0] == 'Q' && document.IsModified(), L"original document remains readable while background save is paused");
        release.set_value(); check(saved.get(), L"background save completes after progress gate releases");
        check(document.IsModified(), L"saving a snapshot does not clear the original document history");
    }
    const auto replacement = root / L"async-replacement.bin";
    Write(source, {'A','B','C'}); document.Open(source.wstring(), &error); document.SetByte(1, 'X'); Write(replacement, {'N','E','W'});
    check(ReplaceFileW(source.c_str(), replacement.c_str(), nullptr, 0, nullptr, nullptr) != FALSE, L"prepare externally replaced source for background recovery");
    snapshot = document.Snapshot(&error);
    check(snapshot != nullptr, L"snapshot of retained original handle supports asynchronous Save As recovery");
    if (snapshot) { control.progress = {}; cancel = false; check(snapshot->Save(target.wstring(), &error, control) && Read(target) == std::vector<std::uint8_t>({'A','X','C'}), L"asynchronous recovery retains the original edited bytes"); }
    if (!failures) std::wcout << L"PASS: native async-save core tests\n";
    return failures;
}

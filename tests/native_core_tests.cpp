#include <Windows.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "../src/core/binary_document.h"
#include "../src/core/diff_engine.h"

int RunFeatureTests(const std::filesystem::path& root);
int RunWorkflowTests(const std::filesystem::path& root);
int RunEditorReviewTests(const std::filesystem::path& root);
int RunDiscoveryTests(const std::filesystem::path& root);
int RunDiscoveryAnalysisTests(const std::filesystem::path& root);
int RunFileSafetyTests(const std::filesystem::path& root);
int RunAsyncSaveTests(const std::filesystem::path& root);
int RunPagingTests(const std::filesystem::path& root);
int RunTemplateTests(const std::filesystem::path& root);
int RunStructuredTests(const std::filesystem::path& root);
int RunCliTests(const std::filesystem::path& root);
int RunRecoveryTests(const std::filesystem::path& root);
int RunPreferenceTests(const std::filesystem::path& root);
int RunFolderToolsTests(const std::filesystem::path& root);
int RunMergeSyncTests(const std::filesystem::path& root);
int RunBinaryToolsTests(const std::filesystem::path& root);
int RunAnalysisTests(const std::filesystem::path& root);
int RunFirmwareTests(const std::filesystem::path& root);
int RunPatternTests(const std::filesystem::path& root);
int RunMergeToolsTests(const std::filesystem::path& root);
int RunPresentationTests(const std::filesystem::path& root);
int RunRegionTests(const std::filesystem::path& root);
int RunTextReportTests(const std::filesystem::path& root);

namespace {

int failures = 0;

void Check(bool condition, const wchar_t* name) {
    if (!condition) {
        std::wcerr << L"FAIL: " << name << L"\n";
        ++failures;
    }
}

std::filesystem::path TempDirectory() {
    wchar_t root[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, root);
    wchar_t name[MAX_PATH] = {};
    GetTempFileNameW(root, L"set", 0, name);
    DeleteFileW(name);
    CreateDirectoryW(name, nullptr);
    return name;
}

void Write(const std::filesystem::path& path, const std::vector<std::uint8_t>& data) {
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(data.data()),
                 static_cast<std::streamsize>(data.size()));
}

void TestBinaryDocument(const std::filesystem::path& root) {
    const auto path = root / L"document.bin";
    Write(path, {'A', 'B', 'C', 'D', 'E'});
    simple_editor::core::BinaryDocument document;
    std::wstring error;
    Check(document.Open(path.wstring(), &error), L"open binary document");
    Check(document.Insert(0, {'1', '2'}), L"insert prefix");
    Check(document.Insert(document.Size(), {'8', '9'}), L"insert suffix");
    Check(document.Delete(3, 2), L"delete range");
    Check(document.Replace(2, 1, {'x', 'y', 'z'}), L"replace changing length");
    std::vector<std::uint8_t> data;
    Check(document.Read(0, static_cast<std::size_t>(document.Size()), &data), L"read edited data");
    Check(data == std::vector<std::uint8_t>({'1', '2', 'x', 'y', 'z', 'D', 'E', '8', '9'}),
          L"piece table content");
    std::uint64_t offset = 0;
    Check(document.Undo(&offset) && offset == 2, L"undo replace");
    Check(document.Redo(&offset) && offset == 2, L"redo replace");
    const bool saved_ok = document.Save(L"", &error);
    if (!saved_ok) std::wcout << L"Save error context: " << error.substr(0, error.find(L": ")) << L'\n';
    Check(saved_ok, L"atomic save");
    document.Close();
    std::ifstream saved(path, std::ios::binary);
    const std::vector<std::uint8_t> actual((std::istreambuf_iterator<char>(saved)),
                                           std::istreambuf_iterator<char>());
    Check(actual == data, L"saved bytes");
}

void TestDiff(const std::filesystem::path& root) {
    const auto left = root / L"left.bin";
    const auto right = root / L"right.bin";
    std::vector<std::uint8_t> base(80);
    for (std::size_t index = 0; index < base.size(); ++index) base[index] = static_cast<std::uint8_t>(index);
    std::vector<std::uint8_t> shifted = base;
    shifted.insert(shifted.begin() + 10, {'X', 'Y', 'Z'});
    shifted[45] = 0xFE;
    Write(left, base);
    Write(right, shifted);
    std::vector<simple_editor::core::Difference> differences;
    std::atomic_bool cancelled(false);
    std::wstring error;
    const bool ok = simple_editor::core::CompareFiles(
        left.wstring(), right.wstring(), true, cancelled,
        [&differences](const simple_editor::core::DifferenceBatch& batch) {
            differences.insert(differences.end(), batch.begin(), batch.end());
            return true;
        }, [](unsigned int) {}, &error);
    Check(ok, L"aligned compare success");
    Check(differences.size() == 4, L"aligned compare count");
    Check(differences[0].left_offset == simple_editor::core::kMissingOffset &&
          differences[0].right_offset == 10, L"aligned insertion address");
    Check(differences.back().left_offset == 42 && differences.back().right_offset == 45,
          L"aligned shifted substitution address");

    differences.clear();
    const bool equal_ok = simple_editor::core::CompareFiles(
        left.wstring(), right.wstring(), false, cancelled,
        [&differences](const simple_editor::core::DifferenceBatch& batch) {
            differences.insert(differences.end(), batch.begin(), batch.end());
            return true;
        }, [](unsigned int) {}, &error);
    Check(equal_ok, L"equal-address compare success");
    Check(differences.size() > 4, L"equal-address compare does not resynchronize");

    cancelled = true;
    const bool cancelled_ok = simple_editor::core::CompareFiles(
        left.wstring(), right.wstring(), true, cancelled,
        [](const simple_editor::core::DifferenceBatch&) { return true; },
        [](unsigned int) {}, &error);
    Check(cancelled_ok, L"compare cancellation is cooperative, not an I/O failure");
}

} // namespace

int RunWorkflowExpansionTests(const std::filesystem::path& root);
int RunLocalizationTests(const std::filesystem::path& root);
int RunDocumentFocusTests(const std::filesystem::path& root);
int RunOutlineSidebarTests(const std::filesystem::path& root);
int wmain(int argc, wchar_t** argv) {
    const auto root = TempDirectory();
    const bool safety_only = argc > 1 && std::wstring(argv[1]) == L"--file-safety";
    const bool async_only = argc > 1 && std::wstring(argv[1]) == L"--async-save";
    const bool merge_only = argc > 1 && std::wstring(argv[1]) == L"--merge-sync";
    if (argc > 1 && std::wstring(argv[1]) == L"--outline-sidebar") failures += RunOutlineSidebarTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--document-focus") failures += RunDocumentFocusTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--localization") failures += RunLocalizationTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--workflow-expansion") failures += RunWorkflowExpansionTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--search-codec") failures += RunFeatureTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--workflows") failures += RunWorkflowTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--editor-review") failures += RunEditorReviewTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--discovery-analysis") failures += RunDiscoveryAnalysisTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--discovery") failures += RunDiscoveryTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--paging") failures += RunPagingTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--templates") failures += RunTemplateTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--structured") failures += RunStructuredTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--cli") failures += RunCliTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--recovery") failures += RunRecoveryTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--preferences") failures += RunPreferenceTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--folder-tools") failures += RunFolderToolsTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--text-reports") failures += RunTextReportTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--regions") { TestDiff(root); failures += RunRegionTests(root); }
    else if (argc > 1 && std::wstring(argv[1]) == L"--presentation") failures += RunPresentationTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--merge-tools") failures += RunMergeToolsTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--patterns") failures += RunPatternTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--firmware") failures += RunFirmwareTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--analysis") failures += RunAnalysisTests(root);
    else if (argc > 1 && std::wstring(argv[1]) == L"--binary-tools") failures += RunBinaryToolsTests(root);
    else if (merge_only) failures += RunMergeSyncTests(root);
    else {
        if (!safety_only && !async_only) { TestBinaryDocument(root); TestDiff(root); failures += RunFeatureTests(root); }
        if (!async_only) failures += RunFileSafetyTests(root);
        if (!safety_only) failures += RunAsyncSaveTests(root);
    }
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    if (failures == 0) {
        std::wcout << L"PASS: native core tests\n";
        return 0;
    }
    return 1;
}


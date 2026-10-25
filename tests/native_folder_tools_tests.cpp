#include "../src/core/folder_sync.h"
#include "../src/core/folder_policy.h"
#include "../src/core/sync_journal.h"
#include "../src/core/workspace_session.h"
#include <fstream>
#include <iostream>
using namespace simple_editor::core;
namespace {
void Write(const std::filesystem::path& path,const std::string& text) { std::filesystem::create_directories(path.parent_path()); std::ofstream stream(path,std::ios::binary); stream << text; }
void Time(const std::filesystem::path& path,std::uint64_t value) { const auto file = CreateFileW(path.c_str(),FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr); FILETIME stamp{static_cast<DWORD>(value),static_cast<DWORD>(value >> 32)}; SetFileTime(file,nullptr,nullptr,&stamp); CloseHandle(file); }
}
int RunFolderToolsTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* text) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << text << L'\n'; if (!ok) ++failures; };
    const auto left = root / L"criteria-left", right = root / L"criteria-right"; Write(left / L"file.txt","AAAA"); Write(right / L"file.txt","BBBB");
    const std::uint64_t time = 133000000000000000; Time(left / L"file.txt",time); Time(right / L"file.txt",time + 10000000);
    FolderOptions options; FolderScan scan; std::wstring error; std::atomic_bool cancel{false};
    check(ScanFolders(left,right,options,cancel,{},&scan,&error) && scan.rows.size() == 1 && scan.rows[0].state == FolderState::Different && scan.rows[0].newness == FolderNewness::RightNewer,L"content criterion detects equal-size differences and newer side");
    options.criterion = FolderCriterion::Size; check(ScanFolders(left,right,options,cancel,{},&scan,&error) && scan.rows[0].state == FolderState::Equal,L"size criterion intentionally ignores byte contents");
    options.criterion = FolderCriterion::SizeTime; check(ScanFolders(left,right,options,cancel,{},&scan,&error) && scan.rows[0].state == FolderState::Different,L"size/time criterion distinguishes exact timestamps");
    options.time_tolerance_seconds = 2; check(ScanFolders(left,right,options,cancel,{},&scan,&error) && scan.rows[0].state == FolderState::Equal && scan.rows[0].newness == FolderNewness::SameTime,L"timestamp tolerance handles coarse filesystem time precision");
    options.criterion = FolderCriterion::Sha256; check(ScanFolders(left,right,options,cancel,{},&scan,&error) && scan.rows[0].state == FolderState::Different,L"SHA-256 comparison checks actual streamed bytes");
    Write(right / L"file.txt","AAAA"); Time(right / L"file.txt",time); check(ScanFolders(left,right,options,cancel,{},&scan,&error) && scan.rows[0].state == FolderState::Equal,L"SHA-256 recognizes identical data");
    options.criterion = FolderCriterion::Content; Write(right / L"file.txt","BBBB"); Time(right / L"file.txt",time); check(ScanFolders(left,right,options,cancel,{},&scan,&error) && scan.rows[0].state == FolderState::Different && scan.rows[0].newness == FolderNewness::SameTime,L"same-time differing contents retain an ambiguous-newness classification");
    cancel = true; check(!ScanFolders(left,right,options,cancel,{},&scan,&error),L"folder criteria retain cancellation"); cancel = false;
    WorkspaceEntry entry; entry.comparison = true; entry.mode = 4; entry.left = left.wstring(); entry.right = right.wstring(); entry.folder_criterion = 3; entry.folder_tolerance = 2; entry.folder_filter = 8; entry.folder_sort = 6; entry.folder_sort_descending = true; std::vector<WorkspaceEntry> loaded;
    check(SaveWorkspaceFile(root / L"criteria.ini",{entry},&error) && LoadWorkspaceFile(root / L"criteria.ini",&loaded,&error) && loaded[0].folder_criterion == 3 && loaded[0].folder_tolerance == 2 && loaded[0].folder_filter == 8 && loaded[0].folder_sort == 6 && loaded[0].folder_sort_descending,L"workspace roundtrip preserves comparison, time, filter and sorting rules");
    entry.folder_criterion = 99; check(!SaveWorkspaceFile(root / L"criteria-invalid.ini",{entry},&error),L"invalid persisted folder criteria are rejected");
    const auto first = root / L"policy-left", second = root / L"policy-right"; Write(first / L"a.txt","same"); Write(second / L"a.txt","same"); Write(first / L"b.txt","base"); Write(second / L"b.txt","base");
    FolderScan source; FolderBaseline baseline,readback; options = {}; ScanFolders(first,second,options,cancel,{},&source,&error);
    check(CaptureFolderBaseline(source,cancel,{},&baseline,&error) && SaveFolderBaseline(root / L"baseline.setb",baseline,&error) && LoadFolderBaseline(root / L"baseline.setb",&readback,&error) && readback.entries.size() == 2,L"baseline records streamed content fingerprints for each side and reloads safely");
    check(!SaveFolderBaseline(first / L"baseline.setb",baseline,&error),L"baseline output cannot enter either comparison root");
    Write(first / L"a.txt","left change"); Write(second / L"b.txt","right change"); ScanFolders(first,second,options,cancel,{},&source,&error); SyncBundle bundle;
    check(BuildPolicySyncPlan(source,{0,1},true,SyncPolicy::BaselineBoth,0,false,&baseline,cancel,{},&bundle,&error) && bundle.conflicts.empty() && bundle.plans.size() == 2 && bundle.plans[0].operations[0].relative == L"a.txt" && bundle.plans[1].operations[0].relative == L"b.txt",L"baseline sync plans independent changes in opposite directions");
    Write(second / L"a.txt","other change"); ScanFolders(first,second,options,cancel,{},&source,&error);
    check(BuildPolicySyncPlan(source,{0},true,SyncPolicy::BaselineBoth,0,false,&baseline,cancel,{},&bundle,&error) && bundle.plans.empty() && bundle.conflicts.size() == 1,L"both-modified files become explicit conflicts without choosing a timestamp winner");
    Write(first / L"a.txt","same"); Write(second / L"a.txt","same"); std::filesystem::remove(first / L"a.txt"); ScanFolders(first,second,options,cancel,{},&source,&error);
    check(BuildPolicySyncPlan(source,{0},true,SyncPolicy::BaselineBoth,0,false,&baseline,cancel,{},&bundle,&error) && bundle.conflicts.size() == 1,L"baseline deletion is held when propagation is disabled");
    check(BuildPolicySyncPlan(source,{0},true,SyncPolicy::BaselineBoth,0,true,&baseline,cancel,{},&bundle,&error) && bundle.conflicts.empty() && bundle.plans.size() == 1 && bundle.plans[0].operations[0].action == SyncAction::RemoveToBackup,L"explicit baseline deletion produces a recoverable removal plan");
    Write(first / L"a.txt","new"); Write(second / L"a.txt","old"); Time(first / L"a.txt",time + 20000000); Time(second / L"a.txt",time); ScanFolders(first,second,options,cancel,{},&source,&error);
    check(BuildPolicySyncPlan(source,{0},true,SyncPolicy::Newer,0,false,nullptr,cancel,{},&bundle,&error) && bundle.plans.size() == 1 && bundle.plans[0].source_root == first,L"directed newer-only policy selects a newer source");
    check(BuildPolicySyncPlan(source,{0},false,SyncPolicy::Newer,0,false,nullptr,cancel,{},&bundle,&error) && bundle.plans.empty(),L"directed newer-only policy does not overwrite a newer target");
    check(BuildPolicySyncPlan(source,{0},false,SyncPolicy::NewerBoth,0,false,nullptr,cancel,{},&bundle,&error) && bundle.plans.size() == 1 && bundle.plans[0].source_root == first,L"newer-both determines direction from timestamps regardless of the invoked arrow");
    Time(first / L"a.txt",time); ScanFolders(first,second,options,cancel,{},&source,&error); check(BuildPolicySyncPlan(source,{0},true,SyncPolicy::NewerBoth,0,false,nullptr,cancel,{},&bundle,&error) && bundle.conflicts.size() == 1 && bundle.plans.empty(),L"same-time different data blocks automatic newer sync");
    FolderBaseline wrong = baseline; wrong.left_root = right; check(!BuildPolicySyncPlan(source,{0},true,SyncPolicy::BaselineBoth,0,false,&wrong,cancel,{},&bundle,&error),L"a baseline for other roots is rejected");
    const auto js = root / L"journal-source", jt = root / L"journal-target"; Write(js / L"a.txt","new-a"); Write(js / L"b.txt","new-b"); Write(jt / L"a.txt","old-a"); Write(jt / L"b.txt","old-b"); Write(jt / L"extra.txt","keep-extra");
    FolderScan journal_scan; ScanFolders(js,jt,{},cancel,{},&journal_scan,&error); SyncPlan original; BuildSyncPlan(journal_scan,{0,1,2},true,true,&original,&error);
    auto applied = ApplySyncPlan(original,cancel,[&](unsigned) { cancel = true; }); cancel = false; SyncJournal journal;
    check(applied.completed == 1 && applied.cancelled && !applied.journal.empty() && LoadSyncJournal(applied.journal,&journal,&error) && journal.entries[0].completed && !journal.entries[1].started,L"cancelled synchronization flushes intent and completion records without claiming pending operations");
    SyncPlan retry; check(BuildSyncRetry(journal,&retry,&error) && retry.operations.size() == 2,L"journal retry includes only pending operations after validating completed output");
    auto retried = ApplySyncPlan(retry,cancel,{}); SyncJournal retried_journal; check(retried.completed == 2 && retried.error.empty() && !std::filesystem::exists(jt / L"extra.txt") && LoadSyncJournal(retried.journal,&retried_journal,&error),L"retry executes the remaining copy/removal and retains its own recovery journal");
    SyncPlan recovery; check(BuildSyncRecovery(retried_journal,&recovery,&error) && recovery.recovery && recovery.operations.size() == 2,L"recovery validates post-sync files and builds reverse operations");
    auto recovered = ApplySyncPlan(recovery,cancel,{}); check(recovered.completed == 2 && recovered.error.empty() && std::filesystem::exists(jt / L"extra.txt"),L"guarded recovery restores removed and overwritten files while retaining replaced outputs");
    check(BuildSyncRecovery(journal,&recovery,&error) && (recovered = ApplySyncPlan(recovery,cancel,{})).completed == 1 && recovered.error.empty(),L"earlier journal recovery restores its independently committed operation");
    std::ifstream original_a(jt / L"a.txt",std::ios::binary), original_b(jt / L"b.txt",std::ios::binary); std::string a_text((std::istreambuf_iterator<char>(original_a)),{}), b_text((std::istreambuf_iterator<char>(original_b)),{});
    check(a_text == "old-a" && b_text == "old-b",L"recovering both journal stages reconstructs original target bytes"); original_a.close(); original_b.close();
    ScanFolders(js,jt,{},cancel,{},&journal_scan,&error); BuildSyncPlan(journal_scan,{0},true,false,&original,&error); applied = ApplySyncPlan(original,cancel,{}); LoadSyncJournal(applied.journal,&journal,&error); Write(jt / L"a.txt","foreign");
    check(!BuildSyncRecovery(journal,&recovery,&error) && !BuildSyncRetry(journal,&retry,&error),L"post-sync external changes block both recovery and retry without overwriting them");
    SyncPlan unsafe; unsafe.source_root = jt; unsafe.target_root = jt; unsafe.recovery = true; check(!IsRecoverySource(unsafe),L"recovery overlap exception is limited to an owned operation data directory");
    const auto guard_source = root / L"guard-source",guard_target = root / L"guard-target"; Write(guard_source / L"file","NEW"); Write(guard_target / L"file","OLD"); FolderScan guard_scan; ScanFolders(guard_source,guard_target,{},cancel,{},&guard_scan,&error); BuildSyncPlan(guard_scan,{0},true,false,&original,&error); applied = ApplySyncPlan(original,cancel,{}); LoadSyncJournal(applied.journal,&journal,&error); BuildSyncRecovery(journal,&recovery,&error); const auto expected = recovery.operations[0].target; Write(guard_target / L"file","BAD"); Time(guard_target / L"file",expected.modified); recovered = ApplySyncPlan(recovery,cancel,{});
    check(recovered.completed == 0 && !recovered.error.empty(),L"recovery rechecks SHA-256 after preview even when an external edit preserves size and timestamp");
    return failures;
}

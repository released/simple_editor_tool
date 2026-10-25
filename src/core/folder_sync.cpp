#include "folder_sync.h"
#include "folder_text_rules.h"
#include "binary_analysis.h"
#include "sync_journal.h"
#include <algorithm>
#include <cwctype>
#include <map>
#include <set>

namespace simple_editor::core {
namespace {
std::wstring Fold(std::wstring value) { for (auto& c : value) c = static_cast<wchar_t>(towlower(c)); return value; }
bool Wildcard(const std::wstring& text, const std::wstring& pattern) {
    std::size_t i = 0, j = 0, star = SIZE_MAX, retry = 0;
    while (i < text.size()) {
        if (j < pattern.size() && (pattern[j] == L'?' || CompareStringOrdinal(&text[i],1,&pattern[j],1,TRUE) == CSTR_EQUAL)) { ++i; ++j; }
        else if (j < pattern.size() && pattern[j] == L'*') { star = j++; retry = i; }
        else if (star != SIZE_MAX) { j = star + 1; i = ++retry; }
        else return false;
    }
    while (j < pattern.size() && pattern[j] == L'*') ++j; return j == pattern.size();
}
bool SafeRelative(const std::filesystem::path& path) {
    if (path.empty() || path.is_absolute() || path.has_root_name()) return false;
    for (const auto& component : path) if (component == L".." || component == L"." || component.empty() || component.wstring().find(L':') != std::wstring::npos) return false;
    return Fold(path.begin()->wstring()) != L".simpleeditortool.backups";
}
bool Separated(const std::filesystem::path& a, const std::filesystem::path& b) {
    FileStamp first,second; std::wstring error;
    if (!InspectFile(a,&first,&error) || !InspectFile(b,&second,&error) || !first.exists || !second.exists) return false;
    const auto ancestor = [&](std::filesystem::path path, const FileStamp& other) {
        for (;;) {
            FileStamp current; if (!InspectFile(path,&current,&error)) return true;
            if (current.exists && current.volume == other.volume && current.identity == other.identity) return true;
            const auto parent = path.parent_path(); if (parent.empty() || parent == path) break; path = parent;
        }
        return false;
    };
    return !ancestor(a,second) && !ancestor(b,first);
}
bool CheckRoot(const std::filesystem::path& root, const FileStamp& expected, std::wstring* error) {
    FileStamp now;
    if (!InspectFile(root, &now, error) || !now.exists || !now.directory || !SameFileState(now, expected)) { if (error && error->empty()) *error = L"Folder root changed since comparison."; return false; }
    return true;
}
bool SameContents(const std::filesystem::path& a, const std::filesystem::path& b, const std::atomic_bool& cancel, bool* equal, std::wstring* error) {
    BinaryDocument first, second; *equal = false;
    if (!first.Open(a.wstring(), error) || !second.Open(b.wstring(), error)) return false;
    if (first.Size() != second.Size()) return true;
    for (std::uint64_t offset = 0; offset < first.Size(); offset += 1024 * 1024) {
        if (cancel) return false;
        std::vector<std::uint8_t> x, y;
        if (!first.Read(offset, 1024 * 1024, &x) || !second.Read(offset, 1024 * 1024, &y)) { if (error) *error = L"File changed or read failed during content comparison."; return false; }
        if (x != y) return true;
    }
    *equal = first.ValidateSource(true, error) && second.ValidateSource(true, error); return *equal;
}
bool Parents(const std::filesystem::path& path, std::wstring* error) {
    if (!CheckPlainPath(path, error)) return false;
    std::error_code code; std::filesystem::create_directories(path, code);
    if (code || !CheckPlainPath(path, error)) { if (error && error->empty()) *error = L"Could not create target directory."; return false; }
    return true;
}
DWORD CALLBACK BackupProgress(LARGE_INTEGER, LARGE_INTEGER, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD, HANDLE, HANDLE, LPVOID context) {
    return static_cast<const std::atomic_bool*>(context)->load() ? PROGRESS_CANCEL : PROGRESS_CONTINUE;
}
}
bool MatchFileFilter(const std::wstring& name, const std::wstring& patterns) {
    for (std::size_t start = 0; start < patterns.size();) {
        const auto end = patterns.find(L';', start); auto pattern = patterns.substr(start, end == std::wstring::npos ? end : end - start);
        const auto first = pattern.find_first_not_of(L" \t"), last = pattern.find_last_not_of(L" \t");
        if (first != std::wstring::npos && Wildcard(name, pattern.substr(first, last - first + 1))) return true;
        if (end == std::wstring::npos) break; start = end + 1;
    }
    return false;
}
FolderNewness CompareFileTimes(const FileStamp& left,const FileStamp& right,unsigned tolerance_seconds) {
    const auto distance = left.modified > right.modified ? left.modified - right.modified : right.modified - left.modified;
    if (distance <= static_cast<std::uint64_t>(tolerance_seconds) * 10000000) return FolderNewness::SameTime;
    return left.modified > right.modified ? FolderNewness::LeftNewer : FolderNewness::RightNewer;
}
bool ScanFolders(const std::filesystem::path& left, const std::filesystem::path& right, const FolderOptions& options,
                  const std::atomic_bool& cancel, const std::function<void(unsigned)>& progress, FolderScan* scan, std::wstring* error) {
    FolderScan result; std::error_code code; FolderTextSettings text_settings; if (!ParseFolderTextSettings(options.text_rules,&text_settings,error)) return false;
    if (static_cast<unsigned>(options.criterion) > 3 || options.time_tolerance_seconds > 86400) { if (error) *error = L"Invalid folder comparison criterion or timestamp tolerance."; return false; }
    result.left_root = std::filesystem::absolute(left, code).lexically_normal(); if (code) return false;
    result.right_root = std::filesystem::absolute(right, code).lexically_normal(); if (code) return false;
    if (!InspectFile(result.left_root, &result.left_identity, error) || !InspectFile(result.right_root, &result.right_identity, error) ||
        !result.left_identity.exists || !result.right_identity.exists || !result.left_identity.directory || !result.right_identity.directory) { if (error && error->empty()) *error = L"Choose two existing folders."; return false; }
    std::map<std::wstring, FolderRow,OrdinalPathLess> rows;
    const auto walk = [&](const std::filesystem::path& root, bool is_left) -> bool {
        std::vector<std::filesystem::path> pending{root};
        while (!pending.empty()) {
            if (cancel) return false;
            auto folder = pending.back(); pending.pop_back(); if (!CheckPlainPath(folder, error)) return false;
            std::error_code failure; std::filesystem::directory_iterator iterator(folder, failure), end;
            if (failure) { if (error) *error = L"Cannot enumerate folder: " + folder.wstring(); return false; }
            while (iterator != end) {
                if (cancel) return false;
                const auto path = iterator->path(), relative = path.lexically_relative(root);
                const auto name = path.filename().wstring();
                if (Fold(name) != L".simpleeditortool.backups") {
                    FileStamp stamp; std::wstring detail; const bool inspected = InspectFile(path, &stamp, &detail);
                    const bool excluded = !options.exclude.empty() && (MatchFileFilter(name, options.exclude) || MatchFileFilter(relative.wstring(), options.exclude));
                    if (!excluded && (stamp.directory || MatchFileFilter(name, options.include) || !inspected)) {
                        auto& row = rows[Fold(relative.wstring())];
                        if (!row.relative.empty() && (is_left ? row.left.exists : row.right.exists)) { if (error) *error = L"Case-colliding paths cannot be synchronized safely."; return false; }
                        row.relative = relative; (is_left ? row.left : row.right) = stamp;
                        if (!inspected) { row.state = FolderState::Error; row.error = detail; }
                        else if (stamp.directory && options.recursive) pending.push_back(path);
                    }
                }
                if (rows.size() > 200000) { if (error) *error = L"Folder comparison is limited to 200,000 entries."; return false; }
                iterator.increment(failure); if (failure) { if (error) *error = L"Folder changed or enumeration failed."; return false; }
            }
        }
        return true;
    };
    if (!walk(result.left_root, true) || !walk(result.right_root, false)) return false;
    std::size_t index = 0;
    for (auto& pair : rows) {
        if (cancel) return false;
        auto& row = pair.second;
        if (row.state != FolderState::Error) {
            if (!row.left.exists) row.state = FolderState::RightOnly;
            else if (!row.right.exists) row.state = FolderState::LeftOnly;
            else if (row.left.directory != row.right.directory) row.state = FolderState::TypeConflict;
            else if (row.left.directory) row.state = FolderState::Equal;
            else {
                bool equal = false;
                row.newness = CompareFileTimes(row.left,row.right,options.time_tolerance_seconds);
                bool read = true;
                if (options.criterion == FolderCriterion::Size) equal = row.left.size == row.right.size;
                else if (options.criterion == FolderCriterion::SizeTime) equal = row.left.size == row.right.size && row.newness == FolderNewness::SameTime;
                else if (options.criterion == FolderCriterion::Sha256) {
                    BinaryDocument first,second; BinaryAnalysis a,b; SaveControl control; control.cancelled = &cancel;
                    read = first.Open((result.left_root / row.relative).wstring(),&row.error) && second.Open((result.right_root / row.relative).wstring(),&row.error) &&
                        AnalyzeBinary(first,0,first.Size(),control,&a,&row.error) && AnalyzeBinary(second,0,second.Size(),control,&b,&row.error);
                    equal = read && a.sha256 == b.sha256;
                } else read = SameContents(result.left_root / row.relative,result.right_root / row.relative,cancel,&equal,&row.error);
                if (read && !EvaluateFolderText(result.left_root / row.relative,result.right_root / row.relative,text_settings,equal && (options.criterion == FolderCriterion::Content || options.criterion == FolderCriterion::Sha256),cancel,&row,error)) return false;
                FileStamp a,b;
                if (read && (!InspectFile(result.left_root / row.relative,&a,&row.error) || !InspectFile(result.right_root / row.relative,&b,&row.error) || !SameFileState(a,row.left) || !SameFileState(b,row.right))) { read = false; row.error = L"File changed during comparison."; }
                row.state = read ? equal ? FolderState::Equal : FolderState::Different : FolderState::Error;
            }
        }
        result.rows.push_back(std::move(row)); if (progress) progress(static_cast<unsigned>(++index * 100 / (std::max<std::size_t>)(1, rows.size())));
    }
    if (!CheckRoot(result.left_root, result.left_identity, error) || !CheckRoot(result.right_root, result.right_identity, error)) return false;
    *scan = std::move(result); return true;
}
bool BuildSyncPlan(const FolderScan& scan, const std::vector<std::size_t>& selected, bool left_to_right, bool mirror,
                    SyncPlan* plan, std::wstring* error) {
    SyncPlan result; result.source_root = left_to_right ? scan.left_root : scan.right_root; result.target_root = left_to_right ? scan.right_root : scan.left_root;
    result.source_identity = left_to_right ? scan.left_identity : scan.right_identity; result.target_identity = left_to_right ? scan.right_identity : scan.left_identity;
    if (!Separated(result.source_root, result.target_root)) { if (error) *error = L"Sync roots must be different and must not contain one another."; return false; }
    std::set<std::size_t> unique(selected.begin(), selected.end());
    std::set<std::wstring,OrdinalPathLess> selected_directories;
    for (auto index : unique) if (index < scan.rows.size() && (scan.rows[index].left.directory || scan.rows[index].right.directory)) selected_directories.insert(Fold(scan.rows[index].relative.wstring()));
    if (!selected_directories.empty()) for (std::size_t index = 0; index < scan.rows.size(); ++index) {
        auto parent = scan.rows[index].relative.parent_path();
        while (!parent.empty()) { if (selected_directories.count(Fold(parent.wstring()))) { unique.insert(index); break; } parent = parent.parent_path(); }
    }
    for (auto index : unique) {
        if (index >= scan.rows.size()) { if (error) *error = L"Invalid selected entry."; return false; }
        const auto& row = scan.rows[index];
        if (!SafeRelative(row.relative) || row.state == FolderState::Error || row.state == FolderState::TypeConflict) { if (error) *error = L"Unsafe path, unreadable entry or file/folder type conflict: " + row.relative.wstring(); return false; }
        const auto source = left_to_right ? row.left : row.right, target = left_to_right ? row.right : row.left;
        if (row.state == FolderState::Equal) continue;
        if (!source.exists) { if (mirror && target.exists) result.operations.push_back({row.relative, SyncAction::RemoveToBackup, source, target}); }
        else if (source.directory) { if (!target.exists) result.operations.push_back({row.relative, SyncAction::CreateDirectory, source, target}); }
        else result.operations.push_back({row.relative, SyncAction::Copy, source, target});
    }
    std::stable_sort(result.operations.begin(), result.operations.end(), [](const SyncOperation& a, const SyncOperation& b) {
        if (a.action == SyncAction::RemoveToBackup && b.action == SyncAction::RemoveToBackup) return a.relative.native().size() > b.relative.native().size();
        if (a.action == SyncAction::RemoveToBackup || b.action == SyncAction::RemoveToBackup) return b.action == SyncAction::RemoveToBackup;
        return a.relative.native().size() < b.relative.native().size();
    });
    *plan = std::move(result); return true;
}
SyncResult ApplySyncPlan(const SyncPlan& plan, const std::atomic_bool& cancel, const std::function<void(unsigned)>& progress,
                         const std::function<bool(const std::filesystem::path&)>& writable) {
    SyncResult result;
    if (!Separated(plan.source_root, plan.target_root) && !IsRecoverySource(plan)) { result.error = L"Sync roots overlap."; return result; }
    if (!CheckRoot(plan.source_root, plan.source_identity, &result.error) || !CheckRoot(plan.target_root, plan.target_identity, &result.error)) return result;
    const auto validate = [&](const SyncOperation& operation) {
        FileStamp source, target;
        if (!SafeRelative(operation.relative) || !InspectFile(plan.source_root / operation.relative, &source, &result.error) ||
            !InspectFile(plan.target_root / operation.relative, &target, &result.error)) return false;
        if (!SameFileState(source, operation.source) || !SameFileState(target, operation.target)) { result.error = L"A file changed since comparison; no stale operation was applied: " + operation.relative.wstring(); return false; }
        for (const auto& side : {std::make_pair(plan.source_root / operation.relative,&operation.source_sha256),std::make_pair(plan.target_root / operation.relative,&operation.target_sha256)}) if (!side.second->empty()) {
            BinaryDocument document; BinaryAnalysis hash; SaveControl control; control.cancelled = &cancel;
            if (!document.Open(side.first.wstring(),&result.error) || !AnalyzeBinary(document,0,document.Size(),control,&hash,&result.error) || hash.sha256 != *side.second) { if (result.error.empty()) result.error = L"Recovery input content changed after preview."; return false; }
        }
        if (writable && !writable(plan.target_root / operation.relative)) { result.error = L"An output has unsaved editor changes; save it or deselect it first."; return false; }
        return true;
    };
    for (const auto& operation : plan.operations) if (!validate(operation)) return result;
    if (plan.operations.empty()) return result;
    static std::atomic_uint64_t sequence{0}; wchar_t token[96]{}; swprintf_s(token, L"%llu-%lu-%llu", GetTickCount64(), GetCurrentProcessId(),++sequence);
    const auto backup_root = plan.target_root / L".SimpleEditorTool.backups", operation_root = backup_root / token;
    if (!Parents(backup_root,&result.error) || !CreateDirectoryW(operation_root.c_str(),nullptr)) { if (result.error.empty()) result.error = L"Could not create an exclusive synchronization backup directory."; return result; }
    result.backup = operation_root / L"data";
    if (!Parents(result.backup,&result.error)) return result;
    SyncJournalWriter journal; if (!journal.Begin(plan,result.backup,&result.error)) return result; result.journal = journal.Path(); bool intended = false; std::size_t active_index = 0;
    for (const auto& operation : plan.operations) {
        if (cancel) { result.cancelled = true; break; }
        if (!CheckRoot(plan.source_root, plan.source_identity, &result.error) || !CheckRoot(plan.target_root, plan.target_identity, &result.error) || !validate(operation)) break;
        active_index = result.completed; if (!journal.Intent(active_index,&result.error)) break; intended = true;
        if (cancel) { result.cancelled = true; break; }
        const auto source = plan.source_root / operation.relative, target = plan.target_root / operation.relative, backup = result.backup / operation.relative;
        if (operation.action == SyncAction::RemoveToBackup) {
            if (operation.target.directory) {
                // Never move a whole tree: unexpected/new files must remain untouched.
                if (!RemoveDirectoryW(target.c_str())) { result.error = L"Directory is no longer empty or could not be removed: " + target.wstring(); break; }
            } else {
                if (!Parents(backup.parent_path(), &result.error) || !MoveFileExW(target.c_str(), backup.c_str(), MOVEFILE_WRITE_THROUGH)) { if (result.error.empty()) result.error = L"Could not move target to recovery backup."; break; }
                FileStamp moved;
                if (!InspectFile(backup, &moved, &result.error) || !SameFileState(moved, operation.target)) {
                    MoveFileExW(backup.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH);
                    result.error = L"Target changed during removal; recovery backup/target retained for review."; break;
                }
            }
        } else if (operation.action == SyncAction::CreateDirectory) {
            if (!Parents(target, &result.error)) break;
        } else {
            if (!Parents(target.parent_path(), &result.error)) break;
            if (operation.target.exists) {
                if (!Parents(backup.parent_path(), &result.error) || !CopyFileExW(target.c_str(), backup.c_str(), BackupProgress, const_cast<std::atomic_bool*>(&cancel), nullptr, COPY_FILE_FAIL_IF_EXISTS)) { result.cancelled = cancel.load(); if (result.error.empty()) result.error = L"Could not preserve output backup; copy aborted."; break; }
                if (!validate(operation)) break;
            }
            BinaryDocument document;
            if (!document.Open(source.wstring(), &result.error)) break;
            FileStamp fresh_source; if (!InspectFile(source, &fresh_source, &result.error) || !SameFileState(fresh_source, operation.source)) { result.error = L"Source changed before copy."; break; }
            std::atomic_bool stop{cancel.load()}; std::wstring commit_error;
            SaveControl control; control.cancelled = &stop;
            control.progress = [&](unsigned percent) {
                if (cancel) stop = true;
                if (percent == 99) {
                    FileStamp source_now, target_now;
                    if (!CheckRoot(plan.source_root, plan.source_identity, &commit_error) || !CheckRoot(plan.target_root, plan.target_identity, &commit_error) ||
                        !InspectFile(source, &source_now, &commit_error) || !InspectFile(target, &target_now, &commit_error) ||
                        !SameFileState(source_now, operation.source) || !SameFileState(target_now, operation.target)) { stop = true; if (commit_error.empty()) commit_error = L"Source or target changed before commit."; }
                    if (!stop && (!operation.source_sha256.empty() || !operation.target_sha256.empty()) && !validate(operation)) { stop = true; commit_error = result.error; }
                }
            };
            if (!document.Save(target.wstring(), &result.error, control)) { result.cancelled = cancel.load(); if (!commit_error.empty()) result.error = commit_error; break; }
        }
        ++result.completed;
        if (!journal.Completed(result.completed - 1,operation,&result.error)) break;
        intended = false; if (progress) progress(static_cast<unsigned>(result.completed * 100 / (std::max<std::size_t>)(1, plan.operations.size())));
    }
    if (intended) journal.Failed(active_index,result.error.empty() ? L"Operation cancelled or interrupted." : result.error);
    return result;
}
} // namespace simple_editor::core

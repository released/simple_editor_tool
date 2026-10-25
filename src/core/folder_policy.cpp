#include "folder_policy.h"
#include "binary_analysis.h"
#include <algorithm>
#include <map>
#include <set>
#include <cstring>
namespace simple_editor::core {
namespace {
bool Relative(const std::filesystem::path& path) {
    if (path.empty() || path.is_absolute() || path.has_root_name() || path.native().find_first_of(L":\r\n\t") != std::wstring::npos) return false;
    for (const auto& part : path) if (part == L".." || part == L"." || part.empty()) return false;
    return CompareStringOrdinal(path.begin()->c_str(),-1,L".SimpleEditorTool.backups",-1,TRUE) != CSTR_EQUAL;
}
bool Same(const BaselineSide& a,const BaselineSide& b) { return a.exists == b.exists && (!a.exists || (a.directory == b.directory && (a.directory || (a.size == b.size && a.sha256 == b.sha256)))); }
bool Digest(const std::filesystem::path& path,const FileStamp& expected,const std::atomic_bool& cancel,BaselineSide* side,std::wstring* error) {
    FileStamp now; if (cancel || !InspectFile(path,&now,error) || !SameFileState(now,expected)) { if (error && error->empty()) *error = L"Folder input changed before hashing."; return false; }
    side->exists = now.exists; side->directory = now.directory; side->size = now.size;
    if (now.exists && !now.directory) { BinaryDocument file; BinaryAnalysis hash; SaveControl control; control.cancelled = &cancel; if (!file.Open(path.wstring(),error) || !AnalyzeBinary(file,0,file.Size(),control,&hash,error)) return false; side->sha256 = hash.sha256; }
    return InspectFile(path,&now,error) && SameFileState(now,expected);
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void Number(std::uint64_t value) { for (unsigned i = 0; i < 8; ++i) { bytes.push_back(static_cast<std::uint8_t>(value)); value >>= 8; } }
    void Text(const std::wstring& text) { Number(text.size()); const auto* data = reinterpret_cast<const std::uint8_t*>(text.data()); bytes.insert(bytes.end(),data,data + text.size() * 2); }
    void Stamp(const FileStamp& stamp) { Number(stamp.volume); Number(stamp.identity); }
    void Side(const BaselineSide& side) { Number(side.exists ? side.directory ? 2 : 1 : 0); Number(side.size); Text(side.sha256); }
};
struct Reader {
    const std::vector<std::uint8_t>& bytes; std::size_t at = 0; bool ok = true;
    std::uint64_t Number() { if (at > bytes.size() || bytes.size() - at < 8) { ok = false; return 0; } std::uint64_t value = 0; for (unsigned i = 0; i < 8; ++i) value |= static_cast<std::uint64_t>(bytes[at++]) << (i * 8); return value; }
    std::wstring Text() { const auto size = Number(); if (!ok || size > 32767 || size > (bytes.size() - at) / 2) { ok = false; return {}; } std::wstring text(static_cast<std::size_t>(size),0); memcpy(text.data(),bytes.data() + at,size * 2); at += static_cast<std::size_t>(size * 2); if (text.find(L'\0') != std::wstring::npos) ok = false; return text; }
    FileStamp Stamp() { FileStamp stamp; stamp.exists = stamp.directory = true; const auto volume = Number(); if (volume > MAXDWORD) ok = false; stamp.volume = static_cast<DWORD>(volume); stamp.identity = Number(); return stamp; }
    BaselineSide Side() { BaselineSide side; const auto state = Number(); if (state > 2) ok = false; side.exists = state != 0; side.directory = state == 2; side.size = Number(); side.sha256 = Text(); if (state == 1 && (side.sha256.size() != 64 || side.sha256.find_first_not_of(L"0123456789ABCDEFabcdef") != std::wstring::npos)) ok = false; if (state != 1 && !side.sha256.empty()) ok = false; return side; }
};
bool Outside(const std::filesystem::path& path,const std::filesystem::path& root) {
    std::error_code code; const auto full = std::filesystem::absolute(path,code).lexically_normal(); if (code) return false;
    FileStamp identity; std::wstring error; if (!InspectFile(root,&identity,&error)) return false;
    for (auto part = full; !part.empty();) { FileStamp stamp; if (!InspectFile(part,&stamp,&error)) return false; if (stamp.exists && stamp.volume == identity.volume && stamp.identity == identity.identity) return false; const auto parent = part.parent_path(); if (parent == part) break; part = parent; } return true;
}
}
bool CaptureFolderBaseline(const FolderScan& scan,const std::atomic_bool& cancel,const std::function<void(unsigned)>& progress,FolderBaseline* output,std::wstring* error) {
    FolderBaseline result; result.left_root = scan.left_root; result.right_root = scan.right_root; result.left_identity = scan.left_identity; result.right_identity = scan.right_identity;
    FileStamp a,b; if (!InspectFile(scan.left_root,&a,error) || !InspectFile(scan.right_root,&b,error) || !SameFileState(a,scan.left_identity) || !SameFileState(b,scan.right_identity)) { if (error && error->empty()) *error = L"Folder roots changed since comparison."; return false; }
    for (const auto& row : scan.rows) {
        if (!Relative(row.relative) || row.state == FolderState::Error || row.state == FolderState::TypeConflict) { if (error) *error = L"Cannot establish a baseline for an unsafe/unreadable/type-conflicting row."; return false; }
        BaselineEntry entry; entry.relative = row.relative;
        if (!Digest(scan.left_root / row.relative,row.left,cancel,&entry.left,error) || !Digest(scan.right_root / row.relative,row.right,cancel,&entry.right,error)) return false;
        result.entries.push_back(std::move(entry)); if (progress) progress(static_cast<unsigned>(result.entries.size() * 100 / (std::max<std::size_t>)(1,scan.rows.size())));
    }
    *output = std::move(result); return true;
}
bool SaveFolderBaseline(const std::filesystem::path& path,const FolderBaseline& baseline,std::wstring* error) {
    if (!Outside(path,baseline.left_root) || !Outside(path,baseline.right_root)) { if (error) *error = L"Save the baseline outside both comparison roots."; return false; }
    Writer writer; writer.Text(L"SimpleEditorToolFolderBaseline1"); writer.Text(baseline.left_root.wstring()); writer.Text(baseline.right_root.wstring()); writer.Stamp(baseline.left_identity); writer.Stamp(baseline.right_identity); writer.Number(baseline.entries.size());
    for (const auto& entry : baseline.entries) { writer.Text(entry.relative.wstring()); writer.Side(entry.left); writer.Side(entry.right); }
    if (writer.bytes.size() > 128 * 1024 * 1024) { if (error) *error = L"Folder baseline exceeds 128 MiB."; return false; }
    return WriteBytesSafely(path,writer.bytes,error);
}
bool LoadFolderBaseline(const std::filesystem::path& path,FolderBaseline* baseline,std::wstring* error) {
    BinaryDocument file; std::vector<std::uint8_t> bytes;
    if (!file.Open(path.wstring(),error) || file.Size() > 128 * 1024 * 1024 || !file.Read(0,static_cast<std::size_t>(file.Size()),&bytes) || !file.ValidateSource(true,error)) { if (error && error->empty()) *error = L"Invalid or oversized folder baseline."; return false; }
    Reader reader{bytes}; FolderBaseline result;
    if (reader.Text() != L"SimpleEditorToolFolderBaseline1") reader.ok = false;
    result.left_root = reader.Text(); result.right_root = reader.Text(); result.left_identity = reader.Stamp(); result.right_identity = reader.Stamp(); const auto count = reader.Number();
    if (count > 200000 || !result.left_root.is_absolute() || !result.right_root.is_absolute()) reader.ok = false;
    std::set<std::wstring,OrdinalPathLess> unique;
    for (std::size_t i = 0; reader.ok && i < count; ++i) { BaselineEntry entry; entry.relative = reader.Text(); entry.left = reader.Side(); entry.right = reader.Side(); if (!Relative(entry.relative) || !unique.insert(entry.relative.wstring()).second) reader.ok = false; result.entries.push_back(std::move(entry)); }
    if (!reader.ok || reader.at != bytes.size()) { if (error) *error = L"Malformed folder baseline; no synchronization plan was created."; return false; }
    *baseline = std::move(result); return true;
}
bool BuildPolicySyncPlan(const FolderScan& scan,const std::vector<std::size_t>& selected,bool left_to_right,SyncPolicy policy,unsigned time_tolerance,bool propagate_deletions,const FolderBaseline* baseline,const std::atomic_bool& cancel,const std::function<void(unsigned)>& progress,SyncBundle* output,std::wstring* error) {
    SyncBundle result;
    if (static_cast<unsigned>(policy) > 4 || time_tolerance > 86400) { if (error) *error = L"Invalid synchronization policy or timestamp tolerance."; return false; }
    if (policy == SyncPolicy::Update || policy == SyncPolicy::Mirror) { SyncPlan plan; if (!BuildSyncPlan(scan,selected,left_to_right,policy == SyncPolicy::Mirror,&plan,error)) return false; result.plans.push_back(std::move(plan)); *output = std::move(result); return true; }
    FolderBaseline current; if (!CaptureFolderBaseline(scan,cancel,progress,&current,error)) return false;
    if (policy == SyncPolicy::BaselineBoth && (!baseline || CompareStringOrdinal(scan.left_root.c_str(),-1,baseline->left_root.c_str(),-1,TRUE) != CSTR_EQUAL || CompareStringOrdinal(scan.right_root.c_str(),-1,baseline->right_root.c_str(),-1,TRUE) != CSTR_EQUAL || !SameFileState(scan.left_identity,baseline->left_identity) || !SameFileState(scan.right_identity,baseline->right_identity))) { if (error) *error = L"Select a baseline for these exact folder roots."; return false; }
    std::map<std::wstring,const BaselineEntry*,OrdinalPathLess> previous; if (baseline) for (const auto& row : baseline->entries) previous[row.relative.wstring()] = &row;
    std::set<std::size_t> unique(selected.begin(),selected.end()); std::set<std::wstring,OrdinalPathLess> directories;
    for (auto index : unique) { if (index >= scan.rows.size()) { if (error) *error = L"Invalid selected folder row."; return false; } if (scan.rows[index].left.directory || scan.rows[index].right.directory) directories.insert(scan.rows[index].relative.wstring()); }
    for (std::size_t i = 0; i < scan.rows.size(); ++i) for (auto parent = scan.rows[i].relative.parent_path(); !parent.empty(); parent = parent.parent_path()) if (directories.count(parent.wstring())) { unique.insert(i); break; }
    FolderScan forward = scan, reverse = scan; forward.rows.clear(); reverse.rows.clear();
    for (auto index : unique) {
        const auto& row = scan.rows[index]; const auto& now = current.entries[index]; if (Same(now.left,now.right)) continue;
        bool direction = left_to_right, remove = false; std::wstring conflict;
        if (policy == SyncPolicy::BaselineBoth) {
            const auto found = previous.find(row.relative.wstring()); const BaselineSide empty; const auto* before = found == previous.end() ? nullptr : found->second;
            const bool a = !Same(now.left,before ? before->left : empty), b = !Same(now.right,before ? before->right : empty);
            if (a && b) conflict = L"Both sides changed since the baseline.";
            else if (!a && !b) conflict = L"Different baseline versions require an explicit side choice.";
            else { direction = a; remove = !(direction ? now.left.exists : now.right.exists); if (remove && !propagate_deletions) conflict = L"Deletion propagation is disabled."; }
        } else if (!now.left.exists || !now.right.exists) {
            if (policy == SyncPolicy::NewerBoth) direction = now.left.exists;
            else if (!(direction ? now.left.exists : now.right.exists)) continue;
        } else {
            const auto newer = CompareFileTimes(row.left,row.right,time_tolerance);
            if (newer == FolderNewness::SameTime) conflict = L"Different contents have indistinguishable timestamps.";
            else if (policy == SyncPolicy::NewerBoth) direction = newer == FolderNewness::LeftNewer;
            else if ((direction && newer != FolderNewness::LeftNewer) || (!direction && newer != FolderNewness::RightNewer)) continue;
        }
        if (!conflict.empty()) { result.conflicts.push_back({row.relative,std::move(conflict)}); continue; }
        auto copy = row; copy.state = !copy.left.exists ? FolderState::RightOnly : !copy.right.exists ? FolderState::LeftOnly : FolderState::Different;
        (direction ? forward.rows : reverse.rows).push_back(std::move(copy));
    }
    for (auto pair : {std::make_pair(&forward,true),std::make_pair(&reverse,false)}) { std::vector<std::size_t> rows; for (std::size_t i = 0; i < pair.first->rows.size(); ++i) rows.push_back(i); SyncPlan plan; if (!BuildSyncPlan(*pair.first,rows,pair.second,policy == SyncPolicy::BaselineBoth && propagate_deletions,&plan,error)) return false; if (!plan.operations.empty()) result.plans.push_back(std::move(plan)); }
    *output = std::move(result); return true;
}
}

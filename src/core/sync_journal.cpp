#include "sync_journal.h"
#include "binary_analysis.h"
#include <algorithm>
#include <cstring>
#include <set>
namespace simple_editor::core {
namespace {
struct Writer {
    std::vector<std::uint8_t> bytes;
    void Number(std::uint64_t value) { for (unsigned i = 0; i < 8; ++i) { bytes.push_back(static_cast<std::uint8_t>(value)); value >>= 8; } }
    void Text(const std::wstring& value) { Number(value.size()); const auto* data = reinterpret_cast<const std::uint8_t*>(value.data()); bytes.insert(bytes.end(),data,data + value.size() * 2); }
    void Stamp(const FileStamp& s) { Number(s.exists); Number(s.directory); Number(s.volume); Number(s.identity); Number(s.size); Number(s.modified); }
};
struct Reader {
    const std::vector<std::uint8_t>& bytes; std::size_t at = 0; bool ok = true;
    std::uint64_t Number() { if (at > bytes.size() || bytes.size() - at < 8) { ok = false; return 0; } std::uint64_t v = 0; for (unsigned i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(bytes[at++]) << (i * 8); return v; }
    std::wstring Text() { const auto size = Number(); if (!ok || size > 32767 || size > (bytes.size() - at) / 2) { ok = false; return {}; } std::wstring text(static_cast<std::size_t>(size),0); memcpy(text.data(),bytes.data() + at,size * 2); at += static_cast<std::size_t>(size * 2); if (text.find(L'\0') != std::wstring::npos) ok = false; return text; }
    FileStamp Stamp() { FileStamp s; const auto exists = Number(),dir = Number(),volume = Number(); if (exists > 1 || dir > 1 || volume > MAXDWORD) ok = false; s.exists = exists != 0; s.directory = dir != 0; s.volume = static_cast<DWORD>(volume); s.identity = Number(); s.size = Number(); s.modified = Number(); return s; }
};
bool Relative(const std::filesystem::path& path) {
    if (path.empty() || path.is_absolute() || path.has_root_name() || path.native().find(L':') != std::wstring::npos) return false;
    for (const auto& part : path) if (part == L".." || part == L"." || part.empty()) return false;
    return CompareStringOrdinal(path.begin()->c_str(),-1,L".SimpleEditorTool.backups",-1,TRUE) != CSTR_EQUAL;
}
bool Digest(const std::filesystem::path& path,FileStamp* stamp,std::wstring* hash,std::wstring* error) {
    if (!InspectFile(path,stamp,error)) return false; hash->clear(); if (!stamp->exists || stamp->directory) return true;
    BinaryDocument document; BinaryAnalysis analysis;
    if (!document.Open(path.wstring(),error) || !AnalyzeBinary(document,0,document.Size(),{},&analysis,error)) return false;
    FileStamp fresh; if (!InspectFile(path,&fresh,error) || !SameFileState(*stamp,fresh)) { if (error) *error = L"File changed while recording its recovery fingerprint."; return false; }
    *hash = analysis.sha256; return true;
}
bool Matches(const std::filesystem::path& path,const FileStamp& expected,const std::wstring& hash,std::wstring* error) {
    FileStamp stamp; std::wstring actual; if (!Digest(path,&stamp,&actual,error)) return false;
    if (!SameFileState(stamp,expected) || actual != hash) { if (error) *error = L"Recovery/retry refused: an input or post-sync output changed: " + path.wstring(); return false; } return true;
}
bool Root(const std::filesystem::path& path,const FileStamp& expected,std::wstring* error) {
    FileStamp now; if (!InspectFile(path,&now,error) || !now.exists || !now.directory || !SameFileState(now,expected)) { if (error && error->empty()) *error = L"Journal folder root changed."; return false; } return true;
}
bool HashSyntax(const FileStamp& stamp,const std::wstring& hash) { return stamp.exists && !stamp.directory ? hash.size() == 64 && hash.find_first_not_of(L"0123456789abcdefABCDEF") == std::wstring::npos : hash.empty(); }
}
SyncJournalWriter::~SyncJournalWriter() { if (file_ != INVALID_HANDLE_VALUE) CloseHandle(file_); }
bool SyncJournalWriter::Write(const std::vector<std::uint8_t>& bytes,std::wstring* error) {
    DWORD written = 0; if (bytes.size() > MAXDWORD || !WriteFile(file_,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr) || written != bytes.size() || !FlushFileBuffers(file_)) { if (error) *error = L"Could not persist the synchronization journal; operation stopped. Backups retained."; return false; } return true;
}
bool SyncJournalWriter::Begin(const SyncPlan& plan,const std::filesystem::path& backup,std::wstring* error) {
    journal_.plan = plan; journal_.backup = backup; journal_.path = backup.parent_path() / L"operations.setj"; journal_.entries.resize(plan.operations.size());
    if (!CheckPlainPath(journal_.path,error)) return false;
    file_ = CreateFileW(journal_.path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (file_ == INVALID_HANDLE_VALUE) { if (error) *error = L"Could not create a new synchronization journal."; return false; }
    Writer writer; writer.Text(L"SimpleEditorToolSyncJournal1"); writer.Text(plan.source_root.wstring()); writer.Text(plan.target_root.wstring()); writer.Stamp(plan.source_identity); writer.Stamp(plan.target_identity); writer.Number(plan.recovery); writer.Number(plan.operations.size());
    for (const auto& op : plan.operations) { writer.Text(op.relative.wstring()); writer.Number(static_cast<unsigned>(op.action)); writer.Stamp(op.source); writer.Stamp(op.target); writer.Text(op.source_sha256); writer.Text(op.target_sha256); if (writer.bytes.size() > 128 * 1024 * 1024) { if (error) *error = L"Synchronization journal manifest exceeds 128 MiB."; return false; } }
    return Write(writer.bytes,error);
}
bool SyncJournalWriter::Record(std::size_t index,unsigned phase,const SyncJournalEntry& entry,std::wstring* error) {
    Writer writer; writer.Number(index); writer.Number(phase); writer.Stamp(entry.post); writer.Stamp(entry.backup); writer.Text(entry.post_hash); writer.Text(entry.backup_hash); writer.Text(entry.error.substr(0,32000)); return Write(writer.bytes,error);
}
bool SyncJournalWriter::Intent(std::size_t index,std::wstring* error) {
    auto& entry = journal_.entries[index]; const auto& op = journal_.plan.operations[index];
    if (!Digest(journal_.plan.target_root / op.relative,&entry.backup,&entry.backup_hash,error) || !SameFileState(entry.backup,op.target)) { if (error && error->empty()) *error = L"Target changed before operation intent was persisted."; return false; }
    if (op.action == SyncAction::Copy && (!Digest(journal_.plan.source_root / op.relative,&entry.post,&entry.post_hash,error) || !SameFileState(entry.post,op.source))) { if (error && error->empty()) *error = L"Source changed before operation intent was persisted."; return false; }
    if ((!op.source_sha256.empty() && entry.post_hash != op.source_sha256) || (!op.target_sha256.empty() && entry.backup_hash != op.target_sha256)) { if (error) *error = L"Recovery content changed before its intent was persisted."; return false; }
    entry.started = true; return Record(index,1,entry,error);
}
bool SyncJournalWriter::Completed(std::size_t index,const SyncOperation& operation,std::wstring* error) {
    auto& entry = journal_.entries[index]; const auto wanted_post = entry.post_hash,wanted_backup = entry.backup_hash;
    if (!Digest(journal_.plan.target_root / operation.relative,&entry.post,&entry.post_hash,error) || !Digest(journal_.backup / operation.relative,&entry.backup,&entry.backup_hash,error)) return false;
    if ((operation.action == SyncAction::Copy && (!entry.post.exists || entry.post.directory || entry.post.size != operation.source.size || entry.post_hash != wanted_post)) ||
        (operation.action == SyncAction::CreateDirectory && (!entry.post.exists || !entry.post.directory)) || (operation.action == SyncAction::RemoveToBackup && entry.post.exists) ||
        (operation.target.exists && !operation.target.directory && (!entry.backup.exists || entry.backup.directory || entry.backup_hash != wanted_backup))) { if (error) *error = L"Committed output or recovery backup no longer matches the recorded operation. Retain backups and recompare."; return false; }
    entry.completed = true; return Record(index,2,entry,error);
}
void SyncJournalWriter::Failed(std::size_t index,const std::wstring& error) { if (index >= journal_.entries.size()) return; auto& entry = journal_.entries[index]; entry.error = error; std::wstring ignored; Record(index,3,entry,&ignored); }
bool IsRecoverySource(const SyncPlan& plan) {
    const auto data = plan.source_root.lexically_normal(); const auto operation = data.parent_path(); const auto backups = operation.parent_path();
    return plan.recovery && data.filename() == L"data" && !operation.filename().empty() && operation.filename().wstring().find_first_not_of(L"0123456789-") == std::wstring::npos && CompareStringOrdinal(backups.filename().c_str(),-1,L".SimpleEditorTool.backups",-1,TRUE) == CSTR_EQUAL && CompareStringOrdinal(backups.parent_path().c_str(),-1,plan.target_root.lexically_normal().c_str(),-1,TRUE) == CSTR_EQUAL;
}
bool LoadSyncJournal(const std::filesystem::path& path,SyncJournal* journal,std::wstring* error) {
    BinaryDocument file; std::vector<std::uint8_t> bytes;
    if (path.filename() != L"operations.setj" || !file.Open(path.wstring(),error) || file.Size() > 256 * 1024 * 1024 || !file.Read(0,static_cast<std::size_t>(file.Size()),&bytes) || !file.ValidateSource(true,error)) { if (error && error->empty()) *error = L"Invalid or oversized synchronization journal."; return false; }
    Reader reader{bytes}; SyncJournal result; std::error_code code; result.path = std::filesystem::absolute(path,code).lexically_normal(); result.backup = result.path.parent_path() / L"data";
    if (reader.Text() != L"SimpleEditorToolSyncJournal1") reader.ok = false;
    result.plan.source_root = reader.Text(); result.plan.target_root = reader.Text(); result.plan.source_identity = reader.Stamp(); result.plan.target_identity = reader.Stamp(); const auto recovery = reader.Number(),count = reader.Number(); result.plan.recovery = recovery != 0;
    if (code || recovery > 1 || count > 200000 || !result.plan.source_root.is_absolute() || !result.plan.target_root.is_absolute() || !result.plan.source_identity.directory || !result.plan.target_identity.directory) reader.ok = false;
    SyncPlan location; location.source_root = result.backup; location.target_root = result.plan.target_root; location.recovery = true; if (!IsRecoverySource(location)) reader.ok = false;
    std::set<std::wstring,OrdinalPathLess> unique;
    for (std::size_t i = 0; reader.ok && i < count; ++i) { SyncOperation op; op.relative = reader.Text(); const auto action = reader.Number(); op.action = static_cast<SyncAction>(action); op.source = reader.Stamp(); op.target = reader.Stamp(); op.source_sha256 = reader.Text(); op.target_sha256 = reader.Text(); if ((!op.source_sha256.empty() && !HashSyntax(op.source,op.source_sha256)) || (!op.target_sha256.empty() && !HashSyntax(op.target,op.target_sha256)) || action > 2 || !Relative(op.relative) || !unique.insert(op.relative.wstring()).second) reader.ok = false; result.plan.operations.push_back(std::move(op)); }
    result.entries.resize(result.plan.operations.size());
    while (reader.ok && reader.at < bytes.size()) {
        const auto index = reader.Number(),phase = reader.Number(); SyncJournalEntry entry; entry.post = reader.Stamp(); entry.backup = reader.Stamp(); entry.post_hash = reader.Text(); entry.backup_hash = reader.Text(); entry.error = reader.Text();
        if (!reader.ok) break;
        if (index >= result.entries.size() || phase < 1 || phase > 3) { reader.ok = false; break; }
        auto& previous = result.entries[static_cast<std::size_t>(index)]; if ((phase == 1 && previous.started) || (phase != 1 && !previous.started) || previous.completed) { reader.ok = false; break; }
        entry.started = true; entry.completed = phase == 2;
        if (entry.completed && (!HashSyntax(entry.post,entry.post_hash) || !HashSyntax(entry.backup,entry.backup_hash))) reader.ok = false;
        previous = std::move(entry);
    }
    if (!reader.ok || reader.at != bytes.size()) { if (error) *error = L"Journal is malformed or incomplete. Retain its backups for manual review."; return false; }
    *journal = std::move(result); return true;
}
bool BuildSyncRetry(const SyncJournal& journal,SyncPlan* plan,std::wstring* error) {
    if (!Root(journal.plan.source_root,journal.plan.source_identity,error) || !Root(journal.plan.target_root,journal.plan.target_identity,error)) return false;
    SyncPlan result = journal.plan; result.operations.clear();
    for (std::size_t i = 0; i < journal.entries.size(); ++i) {
        const auto& entry = journal.entries[i]; const auto& op = journal.plan.operations[i];
        if (entry.completed) { if (!Matches(journal.plan.target_root / op.relative,entry.post,entry.post_hash,error)) return false; continue; }
        FileStamp source,target; if (!InspectFile(journal.plan.source_root / op.relative,&source,error) || !InspectFile(journal.plan.target_root / op.relative,&target,error) || !SameFileState(source,op.source) || !SameFileState(target,op.target)) { if (error) *error = L"An unfinished operation changed or may already have committed. Recompare instead of retrying it."; return false; }
        auto pending = op; if (entry.started) { pending.source_sha256 = entry.post_hash; pending.target_sha256 = entry.backup_hash; } result.operations.push_back(std::move(pending));
    }
    *plan = std::move(result); return true;
}
bool BuildSyncRecovery(const SyncJournal& journal,SyncPlan* plan,std::wstring* error) {
    if (!Root(journal.plan.target_root,journal.plan.target_identity,error)) return false;
    SyncPlan result; result.source_root = journal.backup; result.target_root = journal.plan.target_root; result.target_identity = journal.plan.target_identity; result.recovery = true;
    if (!IsRecoverySource(result) || !InspectFile(result.source_root,&result.source_identity,error) || !result.source_identity.exists || !result.source_identity.directory) { if (error) *error = L"The journal backup directory is unavailable."; return false; }
    for (std::size_t index = journal.entries.size(); index > 0; --index) {
        const auto i = index - 1; const auto& entry = journal.entries[i]; const auto& original = journal.plan.operations[i];
        if (!entry.completed) { if (entry.started) { FileStamp now; if (!InspectFile(journal.plan.target_root / original.relative,&now,error) || !SameFileState(now,original.target)) { if (error) *error = L"An interrupted operation has an uncertain output. Retain backups and recompare before recovery."; return false; } } continue; }
        if (!Matches(journal.plan.target_root / original.relative,entry.post,entry.post_hash,error) || !Matches(journal.backup / original.relative,entry.backup,entry.backup_hash,error)) return false;
        SyncOperation op; op.relative = original.relative; op.source = entry.backup; op.target = entry.post;
        op.source_sha256 = entry.backup_hash; op.target_sha256 = entry.post_hash;
        if (!original.target.exists) op.action = SyncAction::RemoveToBackup;
        else if (original.target.directory) op.action = SyncAction::CreateDirectory;
        else { if (!entry.backup.exists || entry.backup.directory || entry.backup.size != original.target.size) { if (error) *error = L"Required pre-sync backup is missing."; return false; } op.action = SyncAction::Copy; }
        result.operations.push_back(std::move(op));
    }
    *plan = std::move(result); return true;
}
}

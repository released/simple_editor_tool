#include "recovery_store.h"
#include "binary_analysis.h"
#include "line_merge.h"
#include <algorithm>
#include <cstring>
#include <bcrypt.h>
namespace simple_editor::core {
namespace {
constexpr std::uint64_t kDraft = 64ULL * 1024 * 1024,kTotal = 256ULL * 1024 * 1024,kQuota = 768ULL * 1024 * 1024;
struct Writer {
    std::vector<std::uint8_t> bytes;
    void Number(std::uint64_t value) { for (unsigned i = 0; i < 8; ++i) { bytes.push_back(static_cast<std::uint8_t>(value)); value >>= 8; } }
    void Text(const std::wstring& text) { Number(text.size()); const auto* value = reinterpret_cast<const std::uint8_t*>(text.data()); bytes.insert(bytes.end(),value,value + text.size() * 2); }
};
struct Reader {
    const std::vector<std::uint8_t>& bytes; std::size_t at = 0; bool ok = true;
    std::uint64_t Number(std::uint64_t max = UINT64_MAX) { if (at > bytes.size() || bytes.size() - at < 8) { ok = false; return 0; } std::uint64_t value = 0; for (unsigned i = 0; i < 8; ++i) value |= static_cast<std::uint64_t>(bytes[at++]) << (i * 8); if (value > max) ok = false; return value; }
    std::wstring Text() { const auto size = Number(32767); if (!ok || size > (bytes.size() - at) / 2) { ok = false; return {}; } std::wstring value(static_cast<std::size_t>(size),0); memcpy(value.data(),bytes.data() + at,size * 2); at += static_cast<std::size_t>(size * 2); if (value.find(L'\0') != std::wstring::npos) ok = false; return value; }
};
bool Token(const std::wstring& value) { return !value.empty() && value.size() <= 96 && value.find_first_not_of(L"0123456789-") == std::wstring::npos; }
bool Hash(const std::wstring& value) { return value.size() == 64 && value.find_first_not_of(L"0123456789ABCDEFabcdef") == std::wstring::npos; }
bool HashBytes(const std::vector<std::uint8_t>& bytes,std::wstring* output) {
    std::vector<UCHAR> storage; struct Handles { BCRYPT_ALG_HANDLE algorithm = nullptr; BCRYPT_HASH_HANDLE hash = nullptr; ~Handles() { if (hash) BCryptDestroyHash(hash); if (algorithm) BCryptCloseAlgorithmProvider(algorithm,0); } } handles;
    DWORD size = 0,used = 0; if (BCryptOpenAlgorithmProvider(&handles.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0) < 0 || BCryptGetProperty(handles.algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&used,0) < 0) return false;
    storage.resize(size); UCHAR digest[32]{}; if (BCryptCreateHash(handles.algorithm,&handles.hash,storage.data(),size,nullptr,0,0) < 0 || (!bytes.empty() && BCryptHashData(handles.hash,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0) < 0) || BCryptFinishHash(handles.hash,digest,sizeof(digest),0) < 0) return false;
    output->clear(); for (auto byte : digest) { wchar_t text[3]{}; swprintf_s(text,L"%02X",byte); *output += text; } return true;
}
std::filesystem::path Payload(const RecoveryIndex& index,std::size_t i) { return index.root / index.generation / (L"draft-" + std::to_wstring(i) + L".bin"); }
bool Digest(const std::filesystem::path& path,const SaveControl& control,std::wstring* hash,std::uint64_t* size,std::wstring* error) {
    BinaryDocument document; BinaryAnalysis analysis; if (!CheckPlainPath(path,error) || !document.Open(path.wstring(),error) || !AnalyzeBinary(document,0,document.Size(),control,&analysis,error)) return false; *hash = analysis.sha256; *size = document.Size(); return true;
}
bool Current(const std::filesystem::path& root,std::wstring* token,std::wstring* error) {
    FileStamp stamp; if (!InspectFile(root / L"current",&stamp,error)) return false; if (!stamp.exists) { token->clear(); return true; }
    BinaryDocument file; std::vector<std::uint8_t> bytes; if (stamp.directory || stamp.size > 192 || !file.Open((root / L"current").wstring(),error) || !file.Read(0,static_cast<std::size_t>(file.Size()),&bytes) || !file.ValidateSource(true,error) || bytes.size() % 2) { if (error && error->empty()) *error = L"Invalid recovery index pointer; copies retained for review."; return false; }
    token->resize(bytes.size() / 2); if (!bytes.empty()) memcpy(token->data(),bytes.data(),bytes.size()); if (!Token(*token)) { if (error) *error = L"Invalid recovery generation name."; return false; } return true;
}
bool LoadGeneration(const std::filesystem::path& root,const std::wstring& generation,RecoveryIndex* output,std::wstring* error) {
    if (!Token(generation)) return false; BinaryDocument file; std::vector<std::uint8_t> bytes; const auto path = root / generation / L"index.setr";
    if (!CheckPlainPath(path,error) || !file.Open(path.wstring(),error) || file.Size() > 16 * 1024 * 1024 || !file.Read(0,static_cast<std::size_t>(file.Size()),&bytes) || !file.ValidateSource(true,error)) return false;
    Reader reader{bytes}; RecoveryIndex index; index.root = root; index.generation = generation;
    if (reader.Text() != L"SimpleEditorToolRecovery1") reader.ok = false; index.previous = reader.Text(); index.clean_exit = reader.Number(1) != 0; index.workspace_sha256 = reader.Text(); const auto count = reader.Number(200); if ((!index.previous.empty() && (!Token(index.previous) || index.previous == generation)) || !Hash(index.workspace_sha256)) reader.ok = false;
    std::uint64_t total = 0;
    for (std::size_t i = 0; reader.ok && i < count; ++i) {
        RecoveryItem item; item.original_path = reader.Text(); item.label = reader.Text(); item.sha256 = reader.Text(); item.size = reader.Number(kDraft); total += item.size; item.view.page = static_cast<int>(reader.Number(1)); item.view.encoding = static_cast<int>(reader.Number(5));
        for (int j = 0; j < 3; ++j) { item.view.zoom[j] = static_cast<int>(reader.Number(300)); if (item.view.zoom[j] < 50) reader.ok = false; }
        item.view.caret = reader.Number(); item.view.top_row = reader.Number(); item.view.hex_rows[0] = static_cast<unsigned>(reader.Number(32)); item.view.hex_groups[0] = static_cast<unsigned>(reader.Number(8)); item.view.hex_bases[0] = reader.Number();
        if (!Hash(item.sha256) || total > kTotal || (item.view.hex_rows[0] != 8 && item.view.hex_rows[0] != 16 && item.view.hex_rows[0] != 32) || (item.view.hex_groups[0] != 1 && item.view.hex_groups[0] != 2 && item.view.hex_groups[0] != 4 && item.view.hex_groups[0] != 8)) reader.ok = false; index.drafts.push_back(std::move(item));
    }
    if (!reader.ok || reader.at != bytes.size()) { if (error) *error = L"Malformed recovery metadata; original files remain unchanged."; return false; }
    std::wstring hash; std::uint64_t size = 0; const auto workspace = root / generation / L"workspace.ini";
    if (!Digest(workspace,{},&hash,&size,error) || hash != index.workspace_sha256 || !LoadWorkspaceFile(workspace,&index.workspace,error)) { if (error && error->empty()) *error = L"Recovery workspace metadata changed."; return false; }
    *output = std::move(index); return true;
}
bool RemoveGeneration(const RecoveryIndex& index,std::wstring* error) {
    const auto directory = index.root / index.generation; std::vector<std::filesystem::path> files;
    for (std::size_t i = 0; i < index.drafts.size(); ++i) { std::wstring hash; std::uint64_t size = 0; const auto path = Payload(index,i); if (!Digest(path,{},&hash,&size,error) || hash != index.drafts[i].sha256 || size != index.drafts[i].size) { if (error && error->empty()) *error = L"A recovery payload changed; copies retained."; return false; } files.push_back(path); }
    std::wstring hash; std::uint64_t size = 0; const auto workspace = directory / L"workspace.ini"; if (!Digest(workspace,{},&hash,&size,error) || hash != index.workspace_sha256) return false; files.push_back(workspace); files.push_back(directory / L"index.setr");
    std::error_code code; std::size_t count = 0; for (std::filesystem::directory_iterator it(directory,code),end; !code && it != end; it.increment(code)) { if (std::find(files.begin(),files.end(),it->path()) == files.end()) { if (error) *error = L"Unknown recovery files were preserved: " + directory.wstring(); return false; } ++count; }
    if (code || count != files.size() || !CheckPlainPath(directory,error)) return false;
    for (const auto& path : files) if (!DeleteFileW(path.c_str())) { if (error) *error = L"Could not remove an owned recovery file: " + path.wstring(); return false; }
    RemoveDirectoryW(directory.c_str()); return true;
}
bool Usage(const std::filesystem::path& root,std::uint64_t* bytes,std::wstring* error) {
    *bytes = 0; std::error_code code; std::size_t count = 0; std::vector<std::filesystem::path> pending{root};
    while (!pending.empty()) { auto path = pending.back(); pending.pop_back(); if (!CheckPlainPath(path,error)) return false; for (std::filesystem::directory_iterator it(path,code),end; !code && it != end; it.increment(code)) { FileStamp stamp; if (++count > 200000 || !InspectFile(it->path(),&stamp,error)) return false; if (stamp.directory) pending.push_back(it->path()); else { if (stamp.size > kQuota - (std::min)(*bytes,kQuota)) { *bytes = kQuota; return true; } *bytes += stamp.size; } } if (code) { if (error) *error = L"Cannot inspect recovery storage quota."; return false; } } return true;
}
struct Stage {
    std::filesystem::path directory; std::vector<std::pair<std::filesystem::path,FileStamp>> files; bool committed = false,owned = false;
    void Own(const std::filesystem::path& path) { FileStamp stamp; std::wstring ignored; if (InspectFile(path,&stamp,&ignored)) files.push_back({path,stamp}); }
    ~Stage() { if (committed || !owned || directory.empty()) return; std::wstring ignored; if (!CheckPlainPath(directory,&ignored)) return; for (const auto& file : files) { FileStamp now; if (InspectFile(file.first,&now,&ignored) && SameFileState(now,file.second)) DeleteFileW(file.first.c_str()); } RemoveDirectoryW(directory.c_str()); }
};
struct RootLock {
    HANDLE file = INVALID_HANDLE_VALUE;
    ~RootLock() { if (file != INVALID_HANDLE_VALUE) CloseHandle(file); }
    bool Open(const std::filesystem::path& root,std::wstring* error) {
        const auto path = root / L"writer.lock"; if (!CheckPlainPath(path,error)) return false;
        file = CreateFileW(path.c_str(),GENERIC_READ | GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
        LARGE_INTEGER size{}; if (file == INVALID_HANDLE_VALUE || !GetFileSizeEx(file,&size) || size.QuadPart != 0) { if (error) *error = L"Recovery storage is busy or its writer lock is not an owned empty file."; return false; } return true;
    }
};
}
bool LoadRecovery(const std::filesystem::path& root,RecoveryIndex* index,std::wstring* error) { std::wstring token; if (!Current(root,&token,error)) return false; if (token.empty()) { *index = {}; index->root = root; return true; } return LoadGeneration(root,token,index,error); }
bool LoadPreviousRecovery(const RecoveryIndex& current,RecoveryIndex* index,std::wstring* error) { if (!Token(current.previous)) { if (error) *error = L"No previous recovery generation is available."; return false; } return LoadGeneration(current.root,current.previous,index,error); }
RecoverySaveResult SaveRecovery(const std::filesystem::path& root,const std::vector<WorkspaceEntry>& workspace,const std::vector<RecoveryDraft>& drafts,bool clean_exit,const SaveControl& control) {
    RecoverySaveResult result; const auto cancelled = [&]() { return control.cancelled && *control.cancelled; }; std::uint64_t estimate = 2 * 1024 * 1024;
    if (workspace.size() > 200 || drafts.size() > 200) { result.error = L"Recovery supports at most 200 sessions/drafts."; return result; }
    for (const auto& draft : drafts) { const auto size = draft.native_text ? draft.utf8.size() : draft.document ? draft.document->Size() : 0; if (size > kDraft || estimate > kTotal + 2 * 1024 * 1024 - size || draft.original_path.size() > 32767 || draft.label.size() > 32767 || draft.original_path.find(L'\0') != std::wstring::npos || draft.label.find(L'\0') != std::wstring::npos) { result.error = L"Draft recovery is limited to 64 MiB per draft and 256 MiB total, with valid labels/paths. Save oversized drafts explicitly."; return result; } estimate += size; if (draft.view.page < 0 || draft.view.page > 1 || draft.view.encoding < 0 || draft.view.encoding > 5 || (draft.view.hex_rows[0] != 8 && draft.view.hex_rows[0] != 16 && draft.view.hex_rows[0] != 32) || (draft.view.hex_groups[0] != 1 && draft.view.hex_groups[0] != 2 && draft.view.hex_groups[0] != 4 && draft.view.hex_groups[0] != 8)) { result.error = L"Invalid recovery view settings."; return result; } for (auto zoom : draft.view.zoom) if (zoom < 50 || zoom > 300) { result.error = L"Invalid recovery zoom."; return result; } }
    if (cancelled()) { result.cancelled = true; return result; }
    if (!CheckPlainPath(root,&result.error)) return result; std::error_code code; std::filesystem::create_directories(root,code); if (code || !CheckPlainPath(root,&result.error)) { if (result.error.empty()) result.error = L"Cannot create recovery storage."; return result; }
    RootLock lock; if (!lock.Open(root,&result.error)) return result; FileStamp pointer_before; if (!InspectFile(root / L"current",&pointer_before,&result.error)) return result;
    RecoveryIndex previous; if (!LoadRecovery(root,&previous,&result.error)) return result; std::uint64_t usage = 0; if (!Usage(root,&usage,&result.error)) return result; if (estimate > kQuota - (std::min)(usage,kQuota)) { result.error = L"Recovery storage exceeds its 768-MiB staging quota. Review or clear owned snapshots."; return result; }
    static std::atomic_uint64_t sequence{0}; RecoveryIndex index; index.root = root; index.generation = std::to_wstring(GetTickCount64()) + L"-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(++sequence); index.previous = previous.generation; index.clean_exit = clean_exit; index.workspace = workspace; Stage stage; stage.directory = root / index.generation;
    if (!CreateDirectoryW(stage.directory.c_str(),nullptr)) { result.error = L"Cannot create an exclusive recovery generation."; return result; }
    stage.owned = true;
    const auto settings = stage.directory / L"workspace.ini"; if (!SaveWorkspaceFile(settings,workspace,&result.error)) return result; stage.Own(settings); std::uint64_t settings_size = 0; if (!Digest(settings,control,&index.workspace_sha256,&settings_size,&result.error)) { result.cancelled = cancelled(); return result; }
    std::uint64_t total = 0;
    for (std::size_t i = 0; i < drafts.size(); ++i) {
        if (cancelled()) { result.cancelled = true; return result; } const auto& draft = drafts[i]; RecoveryItem item; item.original_path = draft.original_path; item.label = draft.label; item.view = draft.view; const auto destination = Payload(index,i);
        if (draft.native_text) {
            std::vector<std::uint8_t> native(draft.utf8.begin(),draft.utf8.end()),encoded; std::wstring text; TextCodec source,utf8; utf8.encoding = TextEncoding::Utf8;
            if (!DecodeText(native,TextEncoding::Utf8,&text,&source,&result.error)) return result; const auto restored = TextFromEditControl(draft.original_text,text);
            auto output_codec = draft.original_codec; if (!EncodeText(restored,output_codec,true,&encoded,&result.error)) { output_codec = utf8; if (!EncodeText(restored,utf8,false,&encoded,&result.error)) return result; result.error.clear(); }
            if (encoded.size() > kDraft || !WriteBytesSafely(destination,encoded,&result.error)) return result; item.view.encoding = static_cast<int>(output_codec.encoding);
        } else { auto snapshot = draft.document ? draft.document->Snapshot(&result.error) : nullptr; if (!snapshot || !snapshot->Save(destination.wstring(),&result.error,control)) { result.cancelled = cancelled(); if (result.error.empty()) result.error = L"Could not write a draft recovery copy."; return result; } }
        stage.Own(destination); if (!Digest(destination,control,&item.sha256,&item.size,&result.error)) { result.cancelled = cancelled(); return result; } total += item.size; if (total > kTotal) { result.error = L"Encoded draft recovery exceeds 256 MiB."; return result; } index.drafts.push_back(std::move(item)); if (control.progress) control.progress(static_cast<unsigned>((i + 1) * 95 / (std::max<std::size_t>)(1,drafts.size())));
    }
    Writer writer; writer.Text(L"SimpleEditorToolRecovery1"); writer.Text(index.previous); writer.Number(clean_exit); writer.Text(index.workspace_sha256); writer.Number(index.drafts.size());
    for (const auto& item : index.drafts) { writer.Text(item.original_path); writer.Text(item.label); writer.Text(item.sha256); writer.Number(item.size); writer.Number(item.view.page); writer.Number(item.view.encoding); for (auto zoom : item.view.zoom) writer.Number(zoom); writer.Number(item.view.caret); writer.Number(item.view.top_row); writer.Number(item.view.hex_rows[0]); writer.Number(item.view.hex_groups[0]); writer.Number(item.view.hex_bases[0]); }
    const auto manifest = stage.directory / L"index.setr"; if (writer.bytes.size() > 16 * 1024 * 1024 || !WriteBytesSafely(manifest,writer.bytes,&result.error)) return result; stage.Own(manifest);
    if (cancelled()) { result.cancelled = true; return result; }
    const auto* name = reinterpret_cast<const std::uint8_t*>(index.generation.data()); std::vector<std::uint8_t> pointer(name,name + index.generation.size() * 2);
    if (!WriteBytesSafely(root / L"current",pointer,&result.error,{},&pointer_before)) return result; stage.committed = true; result.committed = true; result.index = index;
    if (!previous.previous.empty()) { RecoveryIndex old; std::wstring error; if (!LoadGeneration(root,previous.previous,&old,&error) || !RemoveGeneration(old,&error)) result.warning = error.empty() ? L"An older recovery copy was retained for review." : error; }
    if (control.progress) control.progress(100); return result;
}
bool ReadRecoveryDraft(const RecoveryIndex& index,std::size_t item,const SaveControl& control,std::vector<std::uint8_t>* bytes,std::wstring* error) {
    if (item >= index.drafts.size() || !Token(index.generation) || index.drafts[item].size > kDraft) { if (error) *error = L"Invalid recovery item."; return false; }
    std::wstring hash; std::uint64_t size = 0; const auto path = Payload(index,item); if (!Digest(path,control,&hash,&size,error) || hash != index.drafts[item].sha256 || size != index.drafts[item].size) { if (error && error->empty()) *error = L"Draft recovery content changed or is incomplete."; return false; }
    BinaryDocument file; if (!file.Open(path.wstring(),error) || !file.Read(0,static_cast<std::size_t>(size),bytes) || !file.ValidateSource(true,error)) return false;
    std::wstring actual; if (!HashBytes(*bytes,&actual) || actual != hash) { bytes->clear(); if (error && error->empty()) *error = L"Draft changed while reading; no recovery editor was opened."; return false; } return true;
}
bool ClearRecovery(const std::filesystem::path& root,std::wstring* error) {
    FileStamp directory; if (!InspectFile(root,&directory,error)) return false; if (!directory.exists) return true; RootLock lock; if (!lock.Open(root,error)) return false;
    RecoveryIndex index; if (!LoadRecovery(root,&index,error)) return false; if (index.generation.empty()) return true;
    if (!index.previous.empty()) { FileStamp stamp; if (!InspectFile(root / index.previous,&stamp,error)) return false; if (stamp.exists) { RecoveryIndex previous; if (!LoadGeneration(root,index.previous,&previous,error) || !RemoveGeneration(previous,error)) return false; } }
    if (!RemoveGeneration(index,error)) return false; if (!DeleteFileW((root / L"current").c_str())) { if (error) *error = L"Could not clear the recovery index pointer."; return false; } return true;
}
}

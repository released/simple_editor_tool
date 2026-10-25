#include "../src/core/recovery_store.h"
#include <fstream>
#include <iostream>
using namespace simple_editor::core;
namespace {
void Write(const std::filesystem::path& path,const std::string& text) { std::ofstream out(path,std::ios::binary); out << text; }
std::string Read(const std::filesystem::path& path) { std::ifstream in(path,std::ios::binary); return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()}; }
}
int RunRecoveryTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; }; std::wstring error;
    const auto original = root / L"recovery-original.bin",store = root / L"recovery-store"; Write(original,"ABC"); auto document = std::make_shared<BinaryDocument>(); document->Open(original.wstring(),&error); document->SetByte(1,'!'); RecoveryDraft draft; draft.document = document; draft.original_path = original.wstring(); draft.label = L"Modified binary"; WorkspaceEntry entry; entry.left = original.wstring();
    auto saved = SaveRecovery(store,{entry},{draft},false,{}); RecoveryIndex index;
    check(saved.committed && LoadRecovery(store,&index,&error) && index.drafts.size() == 1 && index.workspace.size() == 1 && !index.clean_exit,L"recovery commits an independent indexed snapshot with workspace metadata");
    std::vector<std::uint8_t> bytes; check(ReadRecoveryDraft(index,0,{},&bytes,&error) && std::string(bytes.begin(),bytes.end()) == "A!C" && Read(original) == "ABC" && document->Path() == original.wstring() && document->IsModified(),L"binary draft recovery preserves original disk bytes and the caller's document state");
    RecoveryDraft text; text.native_text = true; text.label = L"Mixed Unicode text"; text.original_text = L"a\nb\r\nc\r"; text.utf8 = "a\r\nB\r\nc\r\n"; text.original_codec.encoding = TextEncoding::Utf16Be; text.original_codec.bom = true; text.view.page = 1;
    saved = SaveRecovery(store,{entry},{text},false,{}); LoadRecovery(store,&index,&error); std::wstring decoded; TextCodec codec;
    check(saved.committed && ReadRecoveryDraft(index,0,{},&bytes,&error) && DecodeText(bytes,TextEncoding::Auto,&decoded,&codec,&error) && decoded == L"a\nB\r\nc\r" && codec.encoding == TextEncoding::Utf16Be && codec.bom,L"native draft snapshots preserve mixed newlines and original UTF-16/BOM encoding");
    text.original_text = L"x"; text.utf8 = "x\xF0\x9F\x98\x80"; text.original_codec.encoding = TextEncoding::Big5; text.original_codec.code_page = 950; text.original_codec.bom = false; saved = SaveRecovery(store,{entry},{text},false,{}); LoadRecovery(store,&index,&error);
    check(saved.committed && ReadRecoveryDraft(index,0,{},&bytes,&error) && index.drafts[0].view.encoding == static_cast<int>(TextEncoding::Utf8) && DecodeText(bytes,TextEncoding::Utf8,&decoded,&codec,&error) && decoded == L"x😀",L"unencodable drafts use a lossless UTF-8 recovery copy without writing the original");
    const auto current = index.generation; std::atomic_bool cancel{false}; SaveControl control; control.cancelled = &cancel; control.progress = [&](unsigned p) { if (p) cancel = true; }; saved = SaveRecovery(store,{entry},{draft},false,control); LoadRecovery(store,&index,&error);
    check(saved.cancelled && !saved.committed && index.generation == current,L"cancelled recovery staging leaves the previously committed snapshot selected");
    const auto lock = CreateFileW((store / L"writer.lock").c_str(),GENERIC_READ | GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr); saved = SaveRecovery(store,{entry},{draft},false,{}); if (lock != INVALID_HANDLE_VALUE) CloseHandle(lock); check(!saved.committed && !saved.error.empty(),L"recovery rejects concurrent writers instead of replacing their index");
    saved = SaveRecovery(store,{entry},{},true,{}); LoadRecovery(store,&index,&error); check(saved.committed && index.clean_exit && index.drafts.empty() && index.workspace[0].left == original.wstring(),L"clean-exit snapshots retain restore paths without resurrecting discarded drafts");
    const auto unknown = store / index.generation / L"keep-user.txt"; Write(unknown,"preserve"); check(!ClearRecovery(store,&error) && Read(unknown) == "preserve" && std::filesystem::exists(store / index.generation / L"index.setr"),L"clearing snapshots preserves unknown files and their generation metadata"); DeleteFileW(unknown.c_str()); check(ClearRecovery(store,&error) && LoadRecovery(store,&index,&error) && index.generation.empty() && Read(original) == "ABC",L"explicit clearing removes only validated owned snapshots and preserves original files");
    const auto damaged_store = root / L"damaged-recovery"; saved = SaveRecovery(damaged_store,{entry},{draft},false,{}); LoadRecovery(damaged_store,&index,&error); const auto payload = damaged_store / index.generation / L"draft-0.bin"; Write(payload,"BAD"); check(!ReadRecoveryDraft(index,0,{},&bytes,&error) && !ClearRecovery(damaged_store,&error) && Read(payload) == "BAD",L"changed recovery payloads fail SHA-256 validation and are retained for review");
    draft.view.zoom[0] = 1; saved = SaveRecovery(root / L"bad-recovery-view",{entry},{draft},false,{}); check(!saved.committed && !std::filesystem::exists(root / L"bad-recovery-view"),L"invalid recovery views fail before creating storage");
    return failures;
}

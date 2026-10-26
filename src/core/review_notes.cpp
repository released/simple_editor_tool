#include "review_notes.h"
#include "hex_bookmarks.h"
#include <bcrypt.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#pragma comment(lib,"bcrypt.lib")
namespace simple_editor::core {
namespace {
bool ValidFingerprint(const std::wstring& text) { return text.size() == 64 && text.find_first_not_of(L"0123456789ABCDEF") == std::wstring::npos; }
bool Number(const std::wstring& value,std::size_t maximum,std::size_t* output) { if (value.empty() || value.find_first_not_of(L"0123456789") != std::wstring::npos) return false; try { const auto n = std::stoull(value); if (n > maximum) return false; *output = static_cast<std::size_t>(n); return true; } catch (...) { return false; } }
}
bool FingerprintComparison(const TextMergeModel& model,const std::atomic_bool& cancel,std::wstring* output,std::wstring* error) {
    struct Hash { BCRYPT_ALG_HANDLE algorithm = nullptr; BCRYPT_HASH_HANDLE handle = nullptr; ~Hash() { if (handle) BCryptDestroyHash(handle); if (algorithm) BCryptCloseAlgorithmProvider(algorithm,0); } } hash;
    DWORD length = 0,used = 0; if (!output || BCryptOpenAlgorithmProvider(&hash.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0) < 0 || BCryptGetProperty(hash.algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&length),sizeof(length),&used,0) < 0) return false; std::vector<unsigned char> object(length); if (BCryptCreateHash(hash.algorithm,&hash.handle,object.data(),length,nullptr,0,0) < 0) return false;
    const auto bytes = [&](const void* data,std::size_t size) { const auto* at = static_cast<const unsigned char*>(data); while (size) { if (cancel) return false; const auto n = static_cast<ULONG>((std::min<std::size_t>)(size,65536)); if (BCryptHashData(hash.handle,const_cast<PUCHAR>(at),n,0) < 0) return false; at += n; size -= n; } return true; };
    const auto number = [&](std::uint64_t n) { return bytes(&n,sizeof(n)); };
    const auto text = [&](const std::wstring& value) { return number(value.size()) && bytes(value.data(),value.size() * sizeof(wchar_t)); };
    if (!number(1) || !number(model.three_way)) return false;
    for (const auto* lines : {&model.base,&model.left,&model.right}) { if (!number(lines->size())) return false; for (const auto& line : *lines) if (!text(line.text) || !text(line.ending)) return false; }
    if (!number(model.blocks.size())) return false;
    for (const auto& block : model.blocks) for (auto n : {block.base_begin,block.base_end,block.left_begin,block.left_end,block.right_begin,block.right_end,static_cast<std::size_t>(block.changed),static_cast<std::size_t>(block.important),static_cast<std::size_t>(block.conflict)}) if (!number(n)) return false;
    unsigned char digest[32]{}; if (cancel || BCryptFinishHash(hash.handle,digest,32,0) < 0) { if (error) *error = L"Review fingerprint cancelled or could not be calculated."; return false; }
    std::wostringstream result; result << std::uppercase << std::hex << std::setfill(L'0'); for (auto byte : digest) result << std::setw(2) << static_cast<unsigned>(byte); *output = result.str(); return true;
}
bool ParseReviewNotes(const std::wstring& text,ComparisonNotes* output,std::wstring* error) {
    const auto fail = [&]() { if (error) *error = L"Invalid review metadata: version, fingerprint, duplicate field, note or size limit."; return false; };
    if (!output || text.size() > 500000 || !ValidateText(text,error)) return fail(); std::map<std::wstring,std::wstring> fields; std::wstring section;
    for (const auto& line : SplitLines(text)) { if (line.text.empty()) continue; if (line.text.front() == L'[' && line.text.back() == L']') { section = line.text.substr(1,line.text.size() - 2); continue; } const auto equal = line.text.find(L'='); if (equal == std::wstring::npos || !fields.emplace(section + L"/" + line.text.substr(0,equal),line.text.substr(equal + 1)).second) return fail(); }
    ComparisonNotes result; result.fingerprint = fields[L"SimpleEditorToolTextReview/Fingerprint"]; std::size_t count = 0;
    if (fields[L"SimpleEditorToolTextReview/Version"] != L"1" || !ValidFingerprint(result.fingerprint) || !Number(fields[L"SimpleEditorToolTextReview/Count"],1024,&count)) return fail();
    for (std::size_t i = 0; i < count; ++i) { const auto prefix = L"Note" + std::to_wstring(i) + L"/"; std::size_t block = 0,state = 0; ReviewNote note;
        if (!Number(fields[prefix + L"Block"],600000,&block) || !Number(fields[prefix + L"Reviewed"],1,&state) || !DecodeSetting(fields[prefix + L"Text"],&note.text,4096) || (note.text.empty() && !state) || !ValidateText(note.text,error)) return fail(); note.reviewed = state != 0; if (!result.notes.emplace(block,std::move(note)).second) return fail();
    }
    if (fields.size() != 3 + 3 * count) return fail(); *output = std::move(result); return true;
}
bool ReadReviewNotes(const std::filesystem::path& path,ComparisonNotes* output,std::wstring* error) {
    BinaryDocument document; std::vector<std::uint8_t> bytes; std::wstring text; TextCodec codec;
    if (!document.Open(path.wstring(),error) || document.Size() > 1048576 || !document.Read(0,static_cast<std::size_t>(document.Size()),&bytes) || !DecodeText(bytes,TextEncoding::Auto,&text,&codec,error) || !document.ValidateSource(true,error)) return false; return ParseReviewNotes(text,output,error);
}
bool GuardReviewOutput(const std::filesystem::path& path,const std::vector<ProtectedInput>& inputs,FileStamp* stamp,std::wstring* error) {
    if (!InspectFile(path,stamp,error)) return false; std::error_code code; const auto target = std::filesystem::absolute(path,code).lexically_normal(); if (code) return false;
    for (const auto& input : inputs) { if (input.path.empty()) continue; const auto source = std::filesystem::absolute(input.path,code).lexically_normal(); if (code || _wcsicmp(source.c_str(),target.c_str()) == 0 || (stamp->exists && input.identity != std::pair<DWORD,std::uint64_t>{} && input.identity == std::make_pair(stamp->volume,stamp->identity))) { if (error) *error = L"Review output must be separate from every source file and hard link."; return false; } } return true;
}
bool SaveReviewNotes(const std::filesystem::path& path,const ComparisonNotes& notes,const std::vector<ProtectedInput>& inputs,const SaveControl& control,std::wstring* error) {
    if (!ValidFingerprint(notes.fingerprint) || notes.notes.size() > 1024) return false;
    std::wstring text = L"[SimpleEditorToolTextReview]\r\nVersion=1\r\nFingerprint=" + notes.fingerprint + L"\r\nCount=" + std::to_wstring(notes.notes.size()) + L"\r\n"; std::size_t i = 0;
    for (const auto& note : notes.notes) { if (note.first > 600000 || note.second.text.size() > 4096 || (note.second.text.empty() && !note.second.reviewed) || !ValidateText(note.second.text,error)) return false; text += L"\r\n[Note" + std::to_wstring(i++) + L"]\r\nBlock=" + std::to_wstring(note.first) + L"\r\nReviewed=" + std::to_wstring(note.second.reviewed) + L"\r\nText=" + EncodeSetting(note.second.text) + L"\r\n"; if (text.size() > 500000) return false; }
    std::vector<std::uint8_t> bytes; TextCodec codec; codec.encoding = TextEncoding::Utf8; if (!EncodeText(text,codec,false,&bytes,error) || bytes.size() > 1048576) return false; FileStamp expected;
    return GuardReviewOutput(path,inputs,&expected,error) && WriteBytesSafely(path,bytes,error,control,&expected);
}
}

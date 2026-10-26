#include "ips_patch.h"
#include <bcrypt.h>
#include <algorithm>
#include <cwctype>
namespace simple_editor::core {
namespace {
constexpr std::size_t kMaximum = 64 * 1024 * 1024,kRecords = 100000;
bool Fail(std::wstring* error,const wchar_t* message) { if (error) *error = message; return false; }
bool Digest(const std::vector<std::uint8_t>& data,std::wstring* text) {
    BCRYPT_ALG_HANDLE algorithm = nullptr; BCRYPT_HASH_HANDLE hash = nullptr; DWORD size = 0,returned = 0; std::vector<std::uint8_t> object; std::uint8_t digest[32]{};
    bool ok = BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0) >= 0;
    if (ok) ok = BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&returned,0) >= 0;
    if (ok) { object.resize(size); ok = BCryptCreateHash(algorithm,&hash,object.data(),size,nullptr,0,0) >= 0; }
    if (ok && !data.empty()) ok = BCryptHashData(hash,const_cast<PUCHAR>(data.data()),static_cast<ULONG>(data.size()),0) >= 0;
    if (ok) ok = BCryptFinishHash(hash,digest,sizeof(digest),0) >= 0;
    if (hash) BCryptDestroyHash(hash); if (algorithm) BCryptCloseAlgorithmProvider(algorithm,0);
    if (!ok) return false; text->clear(); constexpr wchar_t hex[] = L"0123456789ABCDEF"; for (auto byte : digest) { *text += hex[byte >> 4]; *text += hex[byte & 15]; } return true;
}
void Put(std::vector<std::uint8_t>& bytes,std::uint64_t value,unsigned width) { for (unsigned i = width; i; --i) bytes.push_back(static_cast<std::uint8_t>(value >> ((i - 1) * 8))); }
}
bool BuildIpsPatch(const std::vector<std::uint8_t>& base,const std::vector<std::uint8_t>& updated,bool wide,const std::atomic_bool& cancel,std::vector<std::uint8_t>* output,IpsSummary* summary,std::wstring* error) {
    if (base.size() != updated.size() || base.size() > kMaximum || (!wide && base.size() > 0x1000000)) return Fail(error,L"Patches require equal-length inputs, up to 16 MiB for IPS or 64 MiB for IPS32.");
    const char* header = wide ? "IPS32" : "PATCH"; std::vector<std::uint8_t> bytes(header,header + 5); IpsSummary result; result.ips32 = wide;
    const std::size_t reserved = wide ? 0x45454F46 : 0x454F46;
    for (std::size_t i = 0; i < base.size();) {
        if (cancel) return Fail(error,L"Patch export cancelled."); if (base[i] == updated[i]) { ++i; continue; }
        auto start = i; if (start == reserved) --start; auto end = i;
        while (end < base.size() && end - start < 65535 && base[end] != updated[end]) ++end;
        if (end == reserved && end < base.size() && base[end] != updated[end]) --end;
        const auto length = end - start; const bool repeat = length >= 4 && std::all_of(updated.begin() + start,updated.begin() + end,[&](auto byte) { return byte == updated[start]; });
        Put(bytes,start,wide ? 4 : 3); Put(bytes,repeat ? 0 : length,2);
        if (repeat) { Put(bytes,length,2); bytes.push_back(updated[start]); } else bytes.insert(bytes.end(),updated.begin() + start,updated.begin() + end);
        if (++result.records > kRecords || bytes.size() > kMaximum + 1024 * 1024) return Fail(error,L"Patch exceeds 100,000 records or 65 MiB; narrow the changes."); result.bytes += length; i = end;
    }
    const char* footer = wide ? "EEOF" : "EOF"; bytes.insert(bytes.end(),footer,footer + (wide ? 4 : 3));
    if (cancel || !Digest(base,&result.base_sha256) || !Digest(updated,&result.result_sha256)) return Fail(error,L"Patch export cancelled or SHA-256 unavailable.");
    *output = std::move(bytes); *summary = std::move(result); return true;
}
bool ApplyIpsPatch(BinaryDocument& document,const std::vector<std::uint8_t>& patch,const std::wstring& expected,const SaveControl& control,IpsSummary* summary,std::wstring* error) {
    const auto cancelled = [&]() { return control.cancelled && *control.cancelled; };
    if (patch.size() < 8 || patch.size() > kMaximum + 1024 * 1024 || document.Size() > kMaximum || expected.size() != 64 || !std::all_of(expected.begin(),expected.end(),[](wchar_t ch) { return (ch >= L'0' && ch <= L'9') || (ch >= L'a' && ch <= L'f') || (ch >= L'A' && ch <= L'F'); })) return Fail(error,L"Provide a 64-digit base SHA-256. Input supports equal-length patches up to 65 MiB and documents up to 64 MiB.");
    IpsSummary result; result.ips32 = std::equal(patch.begin(),patch.begin() + 5,"IPS32");
    if (!result.ips32 && !std::equal(patch.begin(),patch.begin() + 5,"PATCH")) return Fail(error,L"Unrecognized IPS / IPS32 header.");
    if (!result.ips32 && document.Size() > 0x1000000) return Fail(error,L"IPS supports documents up to 16 MiB. Use IPS32 for larger documents.");
    std::vector<std::uint8_t> bytes; if (cancelled() || !document.ValidateSource(true,error) || !document.Read(0,static_cast<std::size_t>(document.Size()),&bytes) || !Digest(bytes,&result.base_sha256)) return Fail(error,L"Could not capture and hash the patch base.");
    auto normalized = expected; std::transform(normalized.begin(),normalized.end(),normalized.begin(),[](wchar_t ch) { return static_cast<wchar_t>(towupper(ch)); });
    if (normalized != result.base_sha256) return Fail(error,L"Base SHA-256 mismatch; no patch applied.");
    std::vector<std::pair<std::size_t,std::size_t>> ranges; std::size_t at = 5; const unsigned width = result.ips32 ? 4u : 3u; const char* footer = result.ips32 ? "EEOF" : "EOF"; bool ended = false;
    const auto get = [&](unsigned count,std::uint64_t* value) { if (at > patch.size() || count > patch.size() - at) return false; *value = 0; while (count--) *value = (*value << 8) | patch[at++]; return true; };
    while (at < patch.size()) {
        if (cancelled()) return Fail(error,L"Patch import cancelled; no patch applied.");
        if (width <= patch.size() - at && std::equal(patch.begin() + at,patch.begin() + at + width,footer)) { at += width; ended = true; break; }
        std::uint64_t offset = 0,length = 0; if (!get(width,&offset) || !get(2,&length)) return Fail(error,L"Truncated patch record."); const bool rle = !length;
        if (rle && !get(2,&length)) return Fail(error,L"Truncated RLE record.");
        if (!length || offset > bytes.size() || length > bytes.size() - offset || ++result.records > kRecords || length > kMaximum - result.bytes || (rle ? 1 : length) > patch.size() - at) return Fail(error,L"Invalid record, resize, record count, or decoded-byte limit. Equal-length imports only.");
        const auto first = static_cast<std::size_t>(offset),count = static_cast<std::size_t>(length); ranges.emplace_back(first,first + count); result.bytes += count;
        IpsPreview row; row.offset = first; row.length = count; const auto shown = (std::min<std::size_t>)(16,count); if (result.preview.size() < 200) row.before.assign(bytes.begin() + first,bytes.begin() + first + shown);
        if (rle) std::fill_n(bytes.begin() + first,count,patch[at++]); else { std::copy_n(patch.begin() + at,count,bytes.begin() + first); at += count; }
        if (result.preview.size() < 200) { row.after.assign(bytes.begin() + first,bytes.begin() + first + shown); result.preview.push_back(std::move(row)); }
        if (control.progress) control.progress(static_cast<unsigned>(at * 80 / patch.size()));
    }
    if (!ended || at != patch.size()) return Fail(error,L"Missing terminator, trailing bytes, or a resize extension. No patch applied.");
    std::sort(ranges.begin(),ranges.end()); for (std::size_t i = 1; i < ranges.size(); ++i) if (ranges[i].first < ranges[i - 1].second) return Fail(error,L"Overlapping records are not accepted by this patch importer.");
    if (!Digest(bytes,&result.result_sha256)) return Fail(error,L"Result SHA-256 unavailable.");
    if (cancelled() || !document.ValidateSource(true,error)) return false;
    if (result.base_sha256 != result.result_sha256 && !document.Replace(0,document.Size(),bytes)) return Fail(error,L"Could not apply the captured patch as one Undo action.");
    *summary = std::move(result); return true;
}
}

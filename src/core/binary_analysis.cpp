#include "binary_analysis.h"
#include "leb128.h"
#include <bcrypt.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>

namespace simple_editor::core {
namespace {
bool Bounds(const BinaryDocument& document, std::uint64_t offset, std::uint64_t length, std::wstring* error) { if (!document.IsOpen() || offset > document.Size() || length > document.Size() - offset) { if (error) *error = L"Range must stay within the document."; return false; } return document.ValidateSource(true,error); }
bool Separate(const BinaryDocument& document, const std::filesystem::path& output, std::wstring* error) {
    std::error_code code; const auto target = std::filesystem::absolute(output,code).lexically_normal().wstring(); if (code) return false; const auto source = std::filesystem::absolute(document.Path(),code).lexically_normal().wstring(); FileStamp stamp;
    if (code || !InspectFile(output,&stamp,error)) return false;
    if (CompareStringOrdinal(target.c_str(),-1,source.c_str(),-1,TRUE) == CSTR_EQUAL || (stamp.exists && std::make_pair(stamp.volume,stamp.identity) == document.SourceIdentity())) { if (error) *error = L"Choose an output separate from every input file."; return false; } return true;
}
std::wstring Hex(std::uint64_t value, unsigned width) { wchar_t text[32]{}; swprintf_s(text,L"%0*llX",width,value); return text; }
double Entropy(const std::array<std::uint64_t,256>& counts, std::uint64_t length) { if (!length) return 0; double result = 0; for (auto count : counts) if (count) { const double p = static_cast<double>(count) / length; result -= p * std::log2(p); } return result; }
struct HashHandle { BCRYPT_ALG_HANDLE algorithm = nullptr; BCRYPT_HASH_HANDLE hash = nullptr; ~HashHandle() { if (hash) BCryptDestroyHash(hash); if (algorithm) BCryptCloseAlgorithmProvider(algorithm,0); } };
bool Cancelled(const SaveControl& control, std::wstring* error) { if (control.cancelled && *control.cancelled) { if (error) *error = L"Operation cancelled; original data retained."; return true; } return false; }
}
bool InspectNumbers(const BinaryDocument& document, std::uint64_t offset, bool little, std::vector<InspectorValue>* values, std::wstring* error) {
    if (!Bounds(document,offset,0,error)) return false;
    std::vector<std::uint8_t> data; if (!document.Read(offset,static_cast<std::size_t>((std::min<std::uint64_t>)(8,document.Size() - offset)),&data)) { if (error) *error = L"Could not read inspector bytes."; return false; }
    std::vector<InspectorValue> result;
    for (unsigned width : {1u,2u,4u,8u}) {
        std::uint64_t word = 0; const bool valid = data.size() >= width;
        if (valid) for (unsigned i = 0; i < width; ++i) word |= static_cast<std::uint64_t>(data[i]) << ((little ? i : width - i - 1) * 8);
        std::int64_t signed_word = 0; const auto bits = width * 8;
        if (valid) { std::uint64_t extended = word; if (bits < 64 && (word & (1ULL << (bits - 1)))) extended |= UINT64_MAX << bits; memcpy(&signed_word,&extended,sizeof(signed_word)); }
        result.push_back({L"uint" + std::to_wstring(bits),valid ? std::to_wstring(word) + L" (0x" + Hex(word,width * 2) + L")" : L"Not enough bytes",width,valid});
        result.push_back({L"int" + std::to_wstring(bits),valid ? std::to_wstring(signed_word) : L"Not enough bytes",width,valid});
        if (width == 4 || width == 8) { std::wostringstream text; text << std::setprecision(width == 4 ? 9 : 17); if (width == 4) { const auto raw = static_cast<std::uint32_t>(word); float number; memcpy(&number,&raw,4); text << number; } else { double number; memcpy(&number,&word,8); text << number; } result.push_back({width == 4 ? L"float32" : L"float64",valid ? text.str() : L"Not enough bytes",width,valid}); }
    }
    if (!document.Read(offset,static_cast<std::size_t>((std::min<std::uint64_t>)(10,document.Size() - offset)),&data)) return false;
    for (bool signed_value : {false,true}) { Leb128Value value; const bool valid = DecodeLeb128(data.data(),data.size(),signed_value,&value); result.push_back({signed_value ? L"SLEB128" : L"ULEB128",valid ? (signed_value ? std::to_wstring(value.signed_value) : std::to_wstring(value.unsigned_value)) + L" (" + std::to_wstring(value.length) + L" bytes; endian independent; read-only)" : L"Truncated, unterminated or overflowing 64-bit LEB128",valid ? value.length : 0,valid,false}); }
    if (!document.ValidateSource(true,error)) return false; *values = std::move(result); return true;
}
bool AnalyzeBinary(const BinaryDocument& document, std::uint64_t offset, std::uint64_t length, const SaveControl& control, BinaryAnalysis* analysis, std::wstring* error) {
    if (!Bounds(document,offset,length,error) || Cancelled(control,error)) return false;
    std::vector<std::uint8_t> object; HashHandle provider; DWORD object_bytes = 0, returned = 0;
    if (BCryptOpenAlgorithmProvider(&provider.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0) < 0 || BCryptGetProperty(provider.algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&object_bytes),sizeof(object_bytes),&returned,0) < 0) { if (error) *error = L"Windows SHA-256 provider is unavailable."; return false; }
    object.resize(object_bytes); if (BCryptCreateHash(provider.algorithm,&provider.hash,object.data(),object_bytes,nullptr,0,0) < 0) { if (error) *error = L"Could not initialize SHA-256."; return false; }
    BinaryAnalysis result; result.bytes = length; std::uint32_t crc = 0xFFFFFFFF;
    static const auto table = [] { std::array<std::uint32_t,256> result{}; for (unsigned i = 0; i < 256; ++i) { auto value = i; for (unsigned b = 0; b < 8; ++b) value = (value >> 1) ^ ((value & 1) ? 0xEDB88320u : 0u); result[i] = value; } return result; }();
    std::array<std::uint64_t,256> block{}; const auto block_size = (std::max<std::uint64_t>)(1,length / 512 + (length % 512 != 0)); std::uint64_t block_count = 0;
    for (std::uint64_t done = 0; done < length;) {
        if (Cancelled(control,error)) return false; std::vector<std::uint8_t> bytes;
        if (!document.Read(offset + done,static_cast<std::size_t>((std::min<std::uint64_t>)(1024 * 1024,length - done)),&bytes) || bytes.empty()) { if (error) *error = L"Source changed or could not be read."; return false; }
        if (BCryptHashData(provider.hash,bytes.data(),static_cast<ULONG>(bytes.size()),0) < 0) { if (error) *error = L"SHA-256 update failed."; return false; }
        for (auto byte : bytes) { crc = table[(crc ^ byte) & 255] ^ (crc >> 8); ++result.histogram[byte]; ++block[byte]; if (++block_count == block_size) { result.entropy_blocks.push_back(Entropy(block,block_count)); block.fill(0); block_count = 0; } }
        done += bytes.size(); if (control.progress) control.progress(static_cast<unsigned>(static_cast<long double>(done) * 100 / (std::max<std::uint64_t>)(1,length)));
    }
    if (block_count) result.entropy_blocks.push_back(Entropy(block,block_count)); result.crc32 = crc ^ 0xFFFFFFFF; result.entropy = Entropy(result.histogram,length);
    std::uint8_t digest[32]{}; if (BCryptFinishHash(provider.hash,digest,sizeof(digest),0) < 0) return false;
    for (auto value : digest) result.sha256 += Hex(value,2);
    if (Cancelled(control,error) || !document.ValidateSource(true,error)) return false; *analysis = std::move(result); return true;
}
bool TransformBytes(BinaryDocument& document, std::uint64_t offset, std::uint64_t length, RangeTransform operation, const std::vector<std::uint8_t>& pattern, const SaveControl& control, std::wstring* error) {
    if (!Bounds(document,offset,length,error) || Cancelled(control,error)) return false;
    if (length > 64 * 1024 * 1024) { if (error) *error = L"One undoable range transformation supports up to 64 MiB."; return false; }
    const unsigned word = operation == RangeTransform::Swap2 ? 2 : operation == RangeTransform::Swap4 ? 4 : operation == RangeTransform::Swap8 ? 8 : 1;
    if ((operation == RangeTransform::Fill && pattern.empty()) || length % word) { if (error) *error = L"Provide a fill pattern and a range divisible by the swap width."; return false; }
    std::vector<std::uint8_t> data; if (!document.Read(offset,static_cast<std::size_t>(length),&data)) return false;
    if (operation == RangeTransform::Reverse) std::reverse(data.begin(),data.end());
    else for (std::size_t i = 0; i < data.size(); i += word) { if ((i % 65536) == 0 && Cancelled(control,error)) return false; if (operation == RangeTransform::Fill) data[i] = pattern[i % pattern.size()]; else if (operation == RangeTransform::Invert) data[i] ^= 0xFF; else std::reverse(data.begin() + i,data.begin() + i + word); }
    if (Cancelled(control,error)) return false; return document.Replace(offset,length,data);
}
bool ExtractBytes(const BinaryDocument& document, std::uint64_t offset, std::uint64_t length, const std::filesystem::path& output, const SaveControl& control, std::wstring* error, bool source_array, bool require_new) {
    if (!Bounds(document,offset,length,error) || !Separate(document,output,error) || Cancelled(control,error)) return false;
    StreamOutput stream; if (!stream.Open(output,error,require_new)) return false;
    if (source_array && !stream.Text(L"#include <stddef.h>\r\nstatic const unsigned char data[] = {\r\n",error)) return false;
    for (std::uint64_t done = 0; done < length;) {
        if (Cancelled(control,error)) return false; std::vector<std::uint8_t> data;
        if (!document.Read(offset + done,static_cast<std::size_t>((std::min<std::uint64_t>)(source_array ? 16384 : 1024 * 1024,length - done)),&data) || data.empty()) return false;
        if (source_array) { std::wstring text; text.reserve(data.size() * 6); for (std::size_t i = 0; i < data.size(); ++i) { text += L"0x" + Hex(data[i],2) + L","; text += (done + i + 1) % 16 ? L" " : L"\r\n"; } if (!stream.Text(text,error)) return false; }
        else if (!stream.Write(data.data(),data.size(),error)) return false;
        done += data.size(); if (control.progress) control.progress(static_cast<unsigned>(static_cast<long double>(done) * 80 / (std::max<std::uint64_t>)(1,length)));
    }
    if (source_array && !stream.Text((length ? L"" : L"0") + std::wstring(L"\r\n};\r\nstatic const size_t data_size = ") + std::to_wstring(length) + L";\r\n",error)) return false;
    std::atomic_bool stopped{false}; std::wstring conflict; SaveControl commit; commit.cancelled = &stopped;
    commit.progress = [&](unsigned p) { if (control.progress) control.progress(80 + p / 5); if ((control.cancelled && *control.cancelled) || (p == 99 && !document.ValidateSource(true,&conflict))) stopped = true; };
    const bool success = stream.Commit(commit,error); if (!success && !conflict.empty() && error) *error = conflict; return success;
}
bool JoinBinaryFiles(const std::vector<std::filesystem::path>& paths, const std::filesystem::path& output, const SaveControl& control, std::wstring* error) {
    if (paths.empty() || paths.size() > 1000) { if (error) *error = L"Choose 1-1000 input files in the required order."; return false; }
    std::vector<std::unique_ptr<BinaryDocument>> inputs; std::uint64_t size = 0;
    for (const auto& path : paths) { auto document = std::make_unique<BinaryDocument>(); if (!document->Open(path.wstring(),error) || !Separate(*document,output,error) || document->Size() > UINT64_MAX - size) return false; size += document->Size(); inputs.push_back(std::move(document)); }
    StreamOutput stream; if (!stream.Open(output,error)) return false; std::uint64_t done = 0;
    for (const auto& input : inputs) for (std::uint64_t at = 0; at < input->Size();) { if (Cancelled(control,error)) return false; std::vector<std::uint8_t> data; if (!input->Read(at,static_cast<std::size_t>((std::min<std::uint64_t>)(1024 * 1024,input->Size() - at)),&data) || data.empty() || !stream.Write(data.data(),data.size(),error)) return false; at += data.size(); done += data.size(); if (control.progress) control.progress(static_cast<unsigned>(static_cast<long double>(done) * 80 / (std::max<std::uint64_t>)(1,size))); }
    std::atomic_bool stopped{false}; std::wstring conflict; SaveControl commit; commit.cancelled = &stopped; commit.progress = [&](unsigned p) { if (control.progress) control.progress(80 + p / 5); if (control.cancelled && *control.cancelled) stopped = true; if (p == 99) for (const auto& document : inputs) if (!document->ValidateSource(true,&conflict)) stopped = true; };
    const bool success = stream.Commit(commit,error); if (!success && !conflict.empty() && error) *error = conflict; return success;
}
bool SplitBinaryFile(const BinaryDocument& document, std::uint64_t part, const std::filesystem::path& prefix, const SaveControl& control, std::vector<std::filesystem::path>* completed, std::wstring* error) {
    if (!part || !Bounds(document,0,document.Size(),error) || document.Size() / part + (document.Size() % part != 0) > 100000) { if (error) *error = L"Part size must be positive and generate no more than 100,000 files."; return false; }
    std::vector<std::filesystem::path> targets; const auto count = (std::max<std::uint64_t>)(1,document.Size() / part + (document.Size() % part != 0));
    for (std::uint64_t i = 0; i < count; ++i) { auto path = std::filesystem::path(prefix.wstring() + L".part" + std::to_wstring(i + 1) + L".bin"); FileStamp state; if (!Separate(document,path,error) || !InspectFile(path,&state,error)) return false; if (state.exists) { if (error) *error = L"A split output already exists; choose another prefix."; return false; } targets.push_back(std::move(path)); }
    if (completed) completed->clear();
    for (std::size_t i = 0; i < targets.size(); ++i) { if (!ExtractBytes(document,static_cast<std::uint64_t>(i) * part,(std::min)(part,document.Size() - static_cast<std::uint64_t>(i) * part),targets[i],control,error,false,true)) return false; if (completed) completed->push_back(targets[i]); }
    return true;
}
}

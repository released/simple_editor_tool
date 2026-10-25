#include "firmware_image.h"
#include <algorithm>
#include <sstream>

namespace simple_editor::core {
namespace {
constexpr std::uint64_t address_limit = 0x100000000ULL, data_limit = 64ULL * 1024 * 1024;
int Digit(char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; }
std::wstring Hex(std::uint64_t value,unsigned width) { wchar_t text[32]{}; swprintf_s(text,L"%0*llX",width,value); return text; }
bool Output(const BinaryDocument& source,const std::filesystem::path& output,StreamOutput* stream,std::wstring* error) {
    FileStamp stamp; if (!InspectFile(output,&stamp,error)) return false;
    std::error_code code; const auto a = std::filesystem::absolute(output,code).lexically_normal(); if (code) return false; const auto b = std::filesystem::absolute(source.Path(),code).lexically_normal();
    if (code || _wcsicmp(a.c_str(),b.c_str()) == 0 || (stamp.exists && std::make_pair(stamp.volume,stamp.identity) == source.SourceIdentity())) { if (error) *error = L"Firmware output must be separate from its input."; return false; }
    return stream->Open(output,error);
}
bool Commit(const BinaryDocument& source,StreamOutput& output,const SaveControl& control,std::wstring* error) {
    std::atomic_bool cancel{false}; std::wstring conflict; SaveControl guarded; guarded.cancelled = &cancel;
    guarded.progress = [&](unsigned p) { if (control.progress) control.progress(80 + p / 5); if ((control.cancelled && *control.cancelled) || (p == 99 && !source.ValidateSource(true,&conflict))) cancel = true; };
    const bool ok = output.Commit(guarded,error); if (!ok && !conflict.empty() && error) *error = conflict; return ok;
}
bool Stop(const SaveControl& control,std::wstring* error) { if (control.cancelled && *control.cancelled) { if (error) *error = L"Conversion cancelled before replacement."; return true; } return false; }
std::wstring Intel(unsigned type,unsigned address,const std::vector<std::uint8_t>& data) {
    unsigned sum = static_cast<unsigned>(data.size()) + (address >> 8) + (address & 255) + type; std::wstring line = L":" + Hex(data.size(),2) + Hex(address,4) + Hex(type,2);
    for (auto byte : data) { sum += byte; line += Hex(byte,2); } return line + Hex((256u - (sum & 255)) & 255,2) + L"\r\n";
}
std::wstring SRecord(unsigned type,std::uint32_t address,const std::vector<std::uint8_t>& data) {
    const unsigned width = type == 0 ? 2 : 4, count = width + static_cast<unsigned>(data.size()) + 1; unsigned sum = count; std::wstring line = L"S" + std::to_wstring(type) + Hex(count,2) + Hex(address,width * 2);
    for (unsigned i = 0; i < width; ++i) sum += (address >> (i * 8)) & 255;
    for (auto byte : data) { sum += byte; line += Hex(byte,2); } return line + Hex((~sum) & 255,2) + L"\r\n";
}
}
bool ParseFirmware(const BinaryDocument& source,FirmwareFormat format,const SaveControl& control,FirmwareImage* image,std::wstring* error) {
    if (!source.ValidateSource(true,error) || Stop(control,error)) return false;
    FirmwareImage result; std::uint64_t base = 0, data_records = 0, announced = UINT64_MAX, line_number = 0; bool ended = false, had_header = false, segmented = true;
    const auto fail = [&](const std::wstring& reason) { if (error) *error = L"Record line " + std::to_wstring(line_number) + L": " + reason; return false; };
    const auto line = [&](std::string text) {
        ++line_number; if (!text.empty() && text.back() == '\r') text.pop_back(); if (text.empty()) return true;
        if (ended) return fail(L"unexpected data after the termination record."); if (++result.records > 2000000) return fail(L"record count exceeds 2,000,000.");
        const auto prefix = format == FirmwareFormat::IntelHex ? 1u : 2u;
        if (text.size() < prefix + 4 || (format == FirmwareFormat::IntelHex ? text[0] != ':' : text[0] != 'S' || text[1] < '0' || text[1] > '9') || (text.size() - prefix) % 2) return fail(L"invalid record syntax.");
        std::vector<std::uint8_t> bytes; unsigned sum = 0;
        for (std::size_t i = prefix; i < text.size(); i += 2) { const int high = Digit(text[i]), low = Digit(text[i + 1]); if (high < 0 || low < 0) return fail(L"non-hexadecimal record data."); const auto byte = static_cast<std::uint8_t>(high * 16 + low); bytes.push_back(byte); sum += byte; }
        std::uint64_t address = 0; std::size_t first = 0, count = 0;
        if (format == FirmwareFormat::IntelHex) {
            if (bytes.size() < 5 || bytes.size() != static_cast<std::size_t>(bytes[0]) + 5 || (sum & 255)) return fail(L"length or checksum mismatch.");
            const unsigned type = bytes[3], offset = bytes[1] * 256 + bytes[2]; count = bytes[0]; first = 4;
            if (type == 1) { if (count || offset) return fail(L"invalid EOF record."); ended = true; return true; }
            if (type == 2 || type == 4) { if (count != 2 || offset) return fail(L"invalid extended address record."); segmented = type == 2; base = static_cast<std::uint64_t>(bytes[4] * 256 + bytes[5]) << (segmented ? 4 : 16); return true; }
            if (type == 3 || type == 5) { if (count != 4 || offset || result.has_entry) return fail(L"invalid or repeated entry address."); result.has_entry = true; result.entry = type == 3 ? static_cast<std::uint64_t>(bytes[4] * 256 + bytes[5]) * 16 + bytes[6] * 256 + bytes[7] : (static_cast<std::uint64_t>(bytes[4]) << 24) + (bytes[5] << 16) + (bytes[6] << 8) + bytes[7]; return true; }
            if (type != 0) return fail(L"unsupported Intel record type."); address = base + offset;
            if (segmented && offset + count > 65536) { const auto before = 65536 - offset; if (result.data_bytes + count > data_limit) return fail(L"decoded data exceeds 64 MiB."); result.segments.push_back({address,{bytes.begin() + first,bytes.begin() + first + before}}); result.segments.push_back({base,{bytes.begin() + first + before,bytes.begin() + first + count}}); result.data_bytes += count; ++data_records; return true; }
        } else {
            const unsigned type = text[1] - '0', width = type == 0 || type == 1 || type == 5 || type == 9 ? 2 : type == 2 || type == 6 || type == 8 ? 3 : type == 3 || type == 7 ? 4 : 0;
            if (!width || bytes.size() != static_cast<std::size_t>(bytes[0]) + 1 || bytes[0] < width + 1 || (sum & 255) != 255) return fail(L"invalid type, length or checksum.");
            for (unsigned i = 1; i <= width; ++i) address = address * 256 + bytes[i]; first = width + 1; count = bytes.size() - first - 1;
            if (type == 0) { if (had_header || data_records) return fail(L"header must precede data."); had_header = true; return true; }
            if (type == 5 || type == 6) { if (count || announced != UINT64_MAX) return fail(L"invalid or repeated record count."); announced = address; return true; }
            if (type >= 7) { if (count || result.has_entry) return fail(L"invalid termination address."); result.has_entry = true; result.entry = address; ended = true; return true; }
        }
        if (address >= address_limit || count > address_limit - address || result.data_bytes + count > data_limit) return fail(L"address exceeds 32 bits or decoded data exceeds 64 MiB.");
        ++data_records; if (count) { FirmwareSegment segment; segment.address = address; segment.bytes.assign(bytes.begin() + first,bytes.begin() + first + count); result.segments.push_back(std::move(segment)); result.data_bytes += count; } return true;
    };
    std::string pending;
    for (std::uint64_t offset = 0; offset < source.Size();) {
        if (Stop(control,error)) return false; std::vector<std::uint8_t> bytes; if (!source.Read(offset,static_cast<std::size_t>((std::min<std::uint64_t>)(1024 * 1024,source.Size() - offset)),&bytes) || bytes.empty()) return fail(L"source read failed.");
        for (auto byte : bytes) { if (byte == '\n') { if (!line(std::move(pending))) return false; pending.clear(); } else { if (pending.size() >= 1024) return fail(L"record exceeds 1024 characters."); pending.push_back(static_cast<char>(byte)); } }
        offset += bytes.size(); if (control.progress) control.progress(static_cast<unsigned>(static_cast<long double>(offset) * 100 / (std::max<std::uint64_t>)(1,source.Size())));
    }
    if (!pending.empty() && !line(std::move(pending))) return false;
    if (!ended || (announced != UINT64_MAX && announced != data_records)) return fail(!ended ? L"missing termination record." : L"data record count mismatch.");
    std::sort(result.segments.begin(),result.segments.end(),[](const auto& a,const auto& b) { return a.address < b.address; }); std::vector<FirmwareSegment> merged;
    for (auto& segment : result.segments) { if (!merged.empty()) { const auto end = merged.back().address + merged.back().bytes.size(); if (segment.address < end) return fail(L"overlapping data addresses."); if (segment.address == end) { merged.back().bytes.insert(merged.back().bytes.end(),segment.bytes.begin(),segment.bytes.end()); continue; } } merged.push_back(std::move(segment)); }
    result.segments = std::move(merged); if (!source.ValidateSource(true,error) || Stop(control,error)) return false; *image = std::move(result); return true;
}
bool FirmwareToBinary(const BinaryDocument& source,const FirmwareImage& image,std::uint64_t start,std::uint64_t length,std::uint8_t fill,const std::filesystem::path& path,const SaveControl& control,std::wstring* error) {
    if (start > address_limit || length > address_limit - start || !source.ValidateSource(true,error)) { if (error) *error = L"Binary output range exceeds the 32-bit firmware address space."; return false; }
    std::uint64_t previous = 0; for (const auto& segment : image.segments) { if (segment.address < previous || segment.address >= address_limit || segment.bytes.size() > address_limit - segment.address) { if (error) *error = L"Firmware segment map is invalid."; return false; } previous = segment.address + segment.bytes.size(); }
    StreamOutput output; if (!Output(source,path,&output,error)) return false; std::size_t segment = 0;
    for (std::uint64_t offset = 0; offset < length;) { if (Stop(control,error)) return false; std::vector<std::uint8_t> bytes(static_cast<std::size_t>((std::min<std::uint64_t>)(1024 * 1024,length - offset)),fill); const auto address = start + offset, end = address + bytes.size();
        while (segment < image.segments.size() && image.segments[segment].address + image.segments[segment].bytes.size() <= address) ++segment;
        for (auto i = segment; i < image.segments.size() && image.segments[i].address < end; ++i) { const auto& data = image.segments[i]; const auto begin = (std::max)(address,data.address), last = (std::min)(end,data.address + data.bytes.size()); if (last > begin) std::copy(data.bytes.begin() + (begin - data.address),data.bytes.begin() + (last - data.address),bytes.begin() + (begin - address)); }
        if (!output.Write(bytes.data(),bytes.size(),error)) return false; offset += bytes.size(); if (control.progress) control.progress(static_cast<unsigned>(static_cast<long double>(offset) * 80 / (std::max<std::uint64_t>)(1,length)));
    }
    return Commit(source,output,control,error);
}
bool BinaryToFirmware(const BinaryDocument& source,std::uint64_t base,FirmwareFormat format,const std::filesystem::path& path,const SaveControl& control,std::wstring* error) {
    if (base >= address_limit || source.Size() > address_limit - base || !source.ValidateSource(true,error)) { if (error) *error = L"Base address plus input length must fit the 32-bit firmware address space."; return false; }
    StreamOutput output; if (!Output(source,path,&output,error)) return false; std::uint64_t previous = UINT64_MAX;
    if (format == FirmwareFormat::SRecord && !output.Text(SRecord(0,0,{}),error)) return false;
    for (std::uint64_t offset = 0; offset < source.Size();) { if (Stop(control,error)) return false; std::vector<std::uint8_t> block; if (!source.Read(offset,static_cast<std::size_t>((std::min<std::uint64_t>)(65536,source.Size() - offset)),&block) || block.empty()) { if (error) *error = L"Could not read binary input."; return false; } std::wstring text;
        for (std::size_t i = 0; i < block.size();) { const auto address = base + offset + i; const auto count = (std::min<std::size_t>)({32,block.size() - i,65536 - (address & 65535)});
            if (format == FirmwareFormat::IntelHex && previous != address >> 16) { previous = address >> 16; text += Intel(4,0,{static_cast<std::uint8_t>(previous >> 8),static_cast<std::uint8_t>(previous)}); }
            std::vector<std::uint8_t> bytes(block.begin() + i,block.begin() + i + count); text += format == FirmwareFormat::IntelHex ? Intel(0,static_cast<unsigned>(address & 65535),bytes) : SRecord(3,static_cast<std::uint32_t>(address),bytes); i += count;
        }
        if (!output.Text(text,error)) return false; offset += block.size(); if (control.progress) control.progress(static_cast<unsigned>(static_cast<long double>(offset) * 80 / (std::max<std::uint64_t>)(1,source.Size())));
    }
    if (!output.Text(format == FirmwareFormat::IntelHex ? Intel(1,0,{}) : SRecord(7,static_cast<std::uint32_t>(base),{}),error)) return false;
    return Commit(source,output,control,error);
}
}

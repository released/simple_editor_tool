#include "crc_profiles.h"
#include "line_merge.h"
#include "search.h"
#include <array>
#include <map>
namespace simple_editor::core {
namespace {
std::uint64_t Mask(unsigned width) { return width == 64 ? UINT64_MAX : (1ULL << width) - 1; }
std::uint64_t Reflect(std::uint64_t value,unsigned width) { std::uint64_t result = 0; while (width--) { result = result << 1 | (value & 1); value >>= 1; } return result; }
bool Valid(const CrcProfile& p) { if (p.width < 8 || p.width > 64 || p.name.empty() || p.name.size() > 128 || !ValidateText(p.name)) return false; const auto mask = Mask(p.width); return p.polynomial && p.polynomial <= mask && p.initial <= mask && p.xor_out <= mask; }
}
bool ParseCrcProfile(const std::wstring& text,CrcProfile* output,std::wstring* error) {
    if (text.size() > 4096) return false; CrcProfile p; std::map<std::wstring,std::wstring> settings;
    for (const auto& line : SplitLines(text)) { if (line.text.empty() || line.text[0] == L'#') continue; const auto at = line.text.find(L'='); if (at == std::wstring::npos || !settings.emplace(line.text.substr(0,at),line.text.substr(at + 1)).second) { if (error) *error = L"Use unique name=value settings."; return false; } }
    if (settings.count(L"preset")) { const auto& preset = settings[L"preset"]; if (preset == L"CRC-16/ARC") p = {preset,16,0x8005,0,0,true,true}; else if (preset == L"CRC-16/MODBUS") p = {preset,16,0x8005,0xFFFF,0,true,true}; else if (preset == L"CRC-32/ISCSI") p = {preset,32,0x1EDC6F41,0xFFFFFFFF,0xFFFFFFFF,true,true}; else if (preset == L"CRC-64/ECMA-182") p = {preset,64,0x42F0E1EBA9EA3693,0,0,false,false}; else if (preset != L"CRC-32/ISO-HDLC") { if (error) *error = L"Unknown CRC preset."; return false; } }
    for (const auto& field : settings) { const auto& key = field.first; const auto& value = field.second; if (key == L"preset") continue; if (key == L"name") p.name = value; else if (key == L"refin" || key == L"refout") { if (value != L"yes" && value != L"no") return false; (key == L"refin" ? p.reflect_in : p.reflect_out) = value == L"yes"; } else { std::uint64_t number = 0; if (!ParseAddress(key == L"width" ? L"d:" + value : value,&number)) return false; if (key == L"width" && number <= 64) p.width = static_cast<unsigned>(number); else if (key == L"polynomial") p.polynomial = number; else if (key == L"init") p.initial = number; else if (key == L"xorout") p.xor_out = number; else return false; } }
    if (!Valid(p)) { if (error) *error = L"Choose width 8-64 and a nonzero polynomial and init/xorout values fitting that width. Polynomial omits its top term; hex values are used except width."; return false; } *output = std::move(p); return true;
}
bool CalculateCrc(const BinaryDocument& document,std::uint64_t offset,std::uint64_t length,const CrcProfile& profile,const SaveControl& control,std::uint64_t* output,std::wstring* error) {
    if (!output || !Valid(profile) || offset > document.Size() || length > document.Size() - offset || !document.ValidateSource(true,error)) return false; const auto mask = Mask(profile.width),top = 1ULL << (profile.width - 1); std::array<std::uint64_t,256> table{}; for (unsigned i = 0; i < 256; ++i) { auto crc = static_cast<std::uint64_t>(i) << (profile.width - 8); for (unsigned bit = 0; bit < 8; ++bit) crc = ((crc << 1) ^ ((crc & top) ? profile.polynomial : 0)) & mask; table[i] = crc; }
    auto crc = profile.initial; for (std::uint64_t done = 0; done < length;) { if (control.cancelled && *control.cancelled) return false; const auto amount = static_cast<std::size_t>((std::min<std::uint64_t>)(256 * 1024,length - done)); std::vector<std::uint8_t> bytes; if (!document.Read(offset + done,amount,&bytes) || bytes.size() != amount) return false; for (std::size_t i = 0; i < bytes.size(); ++i) { if (!(i & 4095) && control.cancelled && *control.cancelled) return false; const auto byte = profile.reflect_in ? Reflect(bytes[i],8) : bytes[i]; const auto index = static_cast<unsigned>((crc >> (profile.width - 8)) ^ byte) & 255; crc = ((crc << 8) ^ table[index]) & mask; } done += amount; if (control.progress) control.progress(static_cast<unsigned>(static_cast<long double>(done) * 100 / (std::max<std::uint64_t>)(1,length))); }
    if (control.cancelled && *control.cancelled || !document.ValidateSource(true,error)) return false; *output = ((profile.reflect_out ? Reflect(crc,profile.width) : crc) ^ profile.xor_out) & mask; return true;
}
}

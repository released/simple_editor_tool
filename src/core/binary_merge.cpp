#include "binary_merge.h"
#include <algorithm>
namespace simple_editor::core {
namespace {
bool SignedAdd(std::uint64_t a,std::int64_t delta,std::uint64_t* result) { if (delta >= 0) { const auto n = static_cast<std::uint64_t>(delta); if (a > UINT64_MAX - n) return false; *result = a + n; } else { const auto n = static_cast<std::uint64_t>(-(delta + 1)) + 1; if (a < n) return false; *result = a - n; } return true; }
bool Separate(const BinaryDocument& input,const std::filesystem::path& output,std::wstring* error) { FileStamp stamp; if (!InspectFile(output,&stamp,error)) return false; std::error_code code; const auto a = std::filesystem::absolute(input.Path(),code).lexically_normal(); if (code) return false; const auto b = std::filesystem::absolute(output,code).lexically_normal(); if (code || _wcsicmp(a.c_str(),b.c_str()) == 0 || (stamp.exists && std::make_pair(stamp.volume,stamp.identity) == input.SourceIdentity())) { if (error) *error = L"A binary merge must be saved to a separate output file."; return false; } return true; }
}
bool BuildBinaryPatches(const std::vector<Difference>& differences,const std::set<std::size_t>& selected,bool right_into_left,std::uint64_t left_size,std::uint64_t right_size,std::vector<BinaryPatch>* patches,std::wstring* error) {
    std::vector<BinaryPatch> result; std::int64_t delta = 0; std::uint64_t left_next = 0, right_next = 0;
    if (left_size > INT64_MAX || right_size > INT64_MAX || (!selected.empty() && *selected.rbegin() >= differences.size())) { if (error) *error = L"Invalid selected binary differences."; return false; }
    for (std::size_t i = 0; i < differences.size(); ++i) { const auto& d = differences[i]; const bool has_left = d.left_offset != kMissingOffset, has_right = d.right_offset != kMissingOffset;
        if (!has_left && !has_right) { if (error) *error = L"Difference has no input address."; return false; }
        auto a = d.left_offset, b = d.right_offset;
        if ((has_left && a >= left_size) || (has_right && b >= right_size)) { if (error) *error = L"Difference address exceeds its source."; return false; }
        if (d.left_anchor != kMissingOffset && d.right_anchor != kMissingOffset) { a = d.left_anchor; b = d.right_anchor; if (a > left_size || b > right_size || (has_left && a != d.left_offset) || (has_right && b != d.right_offset)) { if (error) *error = L"Invalid binary alignment anchors."; return false; } delta = static_cast<std::int64_t>(b) - static_cast<std::int64_t>(a); if (!has_left) ++delta; else if (!has_right) --delta; }
        else if (has_left && has_right) delta = static_cast<std::int64_t>(b) - static_cast<std::int64_t>(a);
        else if (has_right) { if (!SignedAdd(b,-delta,&a) || delta == INT64_MAX) { if (error) *error = L"Insertion address mapping overflow."; return false; } ++delta; }
        else { if (!SignedAdd(a,delta,&b) || delta == INT64_MIN) { if (error) *error = L"Removal address mapping overflow."; return false; } --delta; }
        if (a < left_next || b < right_next || a > left_size || b > right_size || (has_left && a == left_size) || (has_right && b == right_size)) { if (error) *error = L"Binary differences are not a valid monotonic alignment."; return false; }
        left_next = a + has_left; right_next = b + has_right;
        if (!selected.count(i)) continue;
        BinaryPatch patch = right_into_left ? BinaryPatch{a,has_left ? 1u : 0u,b,has_right ? 1u : 0u} : BinaryPatch{b,has_right ? 1u : 0u,a,has_left ? 1u : 0u};
        if (!result.empty() && result.back().offset + result.back().remove == patch.offset && result.back().source + result.back().length == patch.source) { result.back().remove += patch.remove; result.back().length += patch.length; }
        else result.push_back(patch);
    }
    *patches = std::move(result); return true;
}
bool MergeBinaryRanges(const BinaryDocument& baseline,const BinaryDocument& donor,const std::vector<BinaryPatch>& patches,const std::filesystem::path& path,const SaveControl& control,std::wstring* error) {
    if (!baseline.ValidateSource(true,error) || !donor.ValidateSource(true,error) || !Separate(baseline,path,error) || !Separate(donor,path,error)) return false;
    std::uint64_t previous = 0, total = baseline.Size();
    for (const auto& patch : patches) { if (patch.offset < previous || patch.offset > baseline.Size() || patch.remove > baseline.Size() - patch.offset || patch.source > donor.Size() || patch.length > donor.Size() - patch.source || patch.length > INT64_MAX - (total - patch.remove)) { if (error) *error = L"Binary merge range is invalid or output is too large."; return false; } previous = patch.offset + patch.remove; total = total - patch.remove + patch.length; }
    StreamOutput output; if (!output.Open(path,error)) return false; std::uint64_t written = 0;
    const auto copy = [&](const BinaryDocument& document,std::uint64_t begin,std::uint64_t length) { while (length) { if (control.cancelled && *control.cancelled) { if (error) *error = L"Binary merge cancelled before replacement."; return false; } std::vector<std::uint8_t> bytes; const auto count = static_cast<std::size_t>((std::min<std::uint64_t>)(1024 * 1024,length)); if (!document.Read(begin,count,&bytes) || bytes.size() != count) { if (error) *error = L"Binary input changed or could not be read."; return false; } if (!output.Write(bytes.data(),bytes.size(),error)) return false; begin += count; length -= count; written += count; if (control.progress) control.progress(static_cast<unsigned>(static_cast<long double>(written) * 80 / (std::max<std::uint64_t>)(1,total))); } return true; };
    previous = 0; for (const auto& patch : patches) { if (!copy(baseline,previous,patch.offset - previous) || !copy(donor,patch.source,patch.length)) return false; previous = patch.offset + patch.remove; }
    if (!copy(baseline,previous,baseline.Size() - previous)) return false;
    std::atomic_bool stopped{false}; std::wstring conflict; SaveControl commit; commit.cancelled = &stopped; commit.progress = [&](unsigned p) { if (control.progress) control.progress(80 + p / 5); if ((control.cancelled && *control.cancelled) || (p == 99 && (!baseline.ValidateSource(true,&conflict) || !donor.ValidateSource(true,&conflict)))) stopped = true; };
    const bool saved = output.Commit(commit,error); if (!saved && !conflict.empty() && error) *error = conflict; return saved;
}
}

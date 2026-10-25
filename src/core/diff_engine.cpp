#include "diff_engine.h"
#include <algorithm>
#include <unordered_map>

namespace simple_editor::core {
namespace {
constexpr std::size_t kChunkSize = 1024 * 1024;
constexpr std::size_t kWindowSize = 64 * 1024;
constexpr std::size_t kAnchorSize = 16;
constexpr std::size_t kBatchSize = 4096;

unsigned int Percent(std::uint64_t offset, std::uint64_t total) {
    return total ? static_cast<unsigned int>(static_cast<long double>(offset) * 100 / total) : 100;
}
bool ReadAt(const BinaryDocument& document, std::uint64_t offset, std::size_t length,
            std::vector<std::uint8_t>* data, std::wstring* error) {
    if (offset <= document.Size() && document.Read(offset, length, data) &&
        data->size() == (std::min<std::uint64_t>)(length, document.Size() - offset)) return true;
    if (document.ValidateSource(false, error) && error) *error = L"Could not read comparison data. The result was discarded.";
    return false;
}
bool Flush(DifferenceBatch* batch, const BatchCallback& callback) {
    if (batch->empty()) return true;
    const bool ok = callback(*batch); batch->clear(); return ok;
}
bool Emit(DifferenceBatch* batch, const Difference& difference, const BatchCallback& callback) {
    batch->push_back(difference);
    return batch->size() < kBatchSize || Flush(batch, callback);
}
std::uint64_t HashAnchor(const std::uint8_t* data) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (std::size_t i = 0; i < kAnchorSize; ++i) { hash ^= data[i]; hash *= 1099511628211ULL; }
    return hash;
}
bool FindAnchor(const std::vector<std::uint8_t>& left, const std::vector<std::uint8_t>& right,
                std::size_t* left_position, std::size_t* right_position,
                const std::atomic_bool& cancelled) {
    if (left.size() < kAnchorSize || right.size() < kAnchorSize) return false;
    std::unordered_map<std::uint64_t, std::size_t> positions; positions.reserve(right.size());
    for (std::size_t p = 0; p + kAnchorSize <= right.size(); ++p) {
        if ((p & 1023) == 0 && cancelled) return false;
        positions.emplace(HashAnchor(right.data() + p), p);
    }
    bool found = false; std::pair<std::size_t, std::size_t> best(SIZE_MAX, SIZE_MAX);
    for (std::size_t p = 0; p + kAnchorSize <= left.size(); ++p) {
        if ((p & 1023) == 0 && cancelled) return false;
        const auto match = positions.find(HashAnchor(left.data() + p));
        if (match == positions.end() || !std::equal(left.begin() + p, left.begin() + p + kAnchorSize, right.begin() + match->second)) continue;
        const std::pair<std::size_t, std::size_t> score((std::max)(p, match->second), p + match->second);
        if (!found || score < best) { found = true; best = score; *left_position = p; *right_position = match->second; }
    }
    return found;
}
bool CompareEqual(const BinaryDocument& left, const BinaryDocument& right,
                  const std::atomic_bool& cancelled, const BatchCallback& callback,
                  const ProgressCallback& progress, std::wstring* error) {
    const auto total = (std::max)(left.Size(), right.Size());
    DifferenceBatch batch; std::vector<std::uint8_t> a, b;
    for (std::uint64_t offset = 0; offset < total && !cancelled; offset += kChunkSize) {
        const auto left_at = (std::min)(offset, left.Size()), right_at = (std::min)(offset, right.Size());
        if (!ReadAt(left, left_at, kChunkSize, &a, error) || !ReadAt(right, right_at, kChunkSize, &b, error)) return false;
        if (a != b) {
            const auto width = (std::max)(a.size(), b.size());
            for (std::size_t i = 0; i < width; ++i) {
                if ((i & 4095) == 0 && cancelled) return true;
                const int av = i < a.size() ? a[i] : -1, bv = i < b.size() ? b[i] : -1;
                if (av != bv && !Emit(&batch, {av < 0 ? kMissingOffset : offset + i,
                    bv < 0 ? kMissingOffset : offset + i, av, bv,(std::min<std::uint64_t>)(offset + i,left.Size()),(std::min<std::uint64_t>)(offset + i,right.Size())}, callback)) return false;
            }
        }
        progress(Percent((std::min)(total, offset + kChunkSize), total));
    }
    return cancelled || Flush(&batch, callback);
}
bool CompareAligned(const BinaryDocument& left, const BinaryDocument& right,
                    const std::atomic_bool& cancelled, const BatchCallback& callback,
                    const ProgressCallback& progress, std::wstring* error) {
    const auto total = (std::max)(left.Size(), right.Size());
    std::uint64_t left_offset = 0, right_offset = 0;
    DifferenceBatch batch; std::vector<std::uint8_t> a, b;
    while ((left_offset < left.Size() || right_offset < right.Size()) && !cancelled) {
        if (!ReadAt(left, left_offset, kWindowSize, &a, error) || !ReadAt(right, right_offset, kWindowSize, &b, error)) return false;
        if (a == b) {
            left_offset += a.size(); right_offset += b.size();
            progress(Percent((std::max)(left_offset, right_offset), total)); continue;
        }
        std::size_t common = 0;
        while (common < (std::min)(a.size(), b.size()) && a[common] == b[common]) ++common;
        left_offset += common; right_offset += common;
        a.erase(a.begin(), a.begin() + common); b.erase(b.begin(), b.begin() + common);
        std::size_t left_shift = 0, right_shift = 0;
        if (a.empty()) right_shift = b.size();
        else if (b.empty()) left_shift = a.size();
        else if (!FindAnchor(a, b, &left_shift, &right_shift, cancelled)) left_shift = right_shift = (std::min)({a.size(), b.size(), kWindowSize / 2});
        else if (!left_shift && !right_shift) left_shift = right_shift = 1;
        if (cancelled) return true;
        const auto paired = (std::min)(left_shift, right_shift);
        for (std::size_t i = 0; i < paired; ++i) {
            if ((i & 4095) == 0 && cancelled) return true;
            if (a[i] != b[i] && !Emit(&batch, {left_offset + i, right_offset + i, a[i], b[i],left_offset + i,right_offset + i}, callback)) return false;
        }
        for (std::size_t i = paired; i < left_shift; ++i) {
            if ((i & 4095) == 0 && cancelled) return true;
            if (!Emit(&batch, {left_offset + i, kMissingOffset, a[i], -1,left_offset + i,right_offset + paired}, callback)) return false;
        }
        for (std::size_t i = paired; i < right_shift; ++i) {
            if ((i & 4095) == 0 && cancelled) return true;
            if (!Emit(&batch, {kMissingOffset, right_offset + i, -1, b[i],left_offset + paired,right_offset + i}, callback)) return false;
        }
        left_offset += left_shift; right_offset += right_shift;
        progress(Percent((std::max)(left_offset, right_offset), total));
    }
    return cancelled || Flush(&batch, callback);
}
} // namespace

bool CompareDocuments(const BinaryDocument& left, const BinaryDocument& right,
                      bool insertion_aware, const std::atomic_bool& cancelled,
                      const BatchCallback& batch_callback,
                      const ProgressCallback& progress_callback, std::wstring* error) {
    if (error) error->clear();
    if (cancelled) return true;
    if (!left.ValidateSource(false, error) || !right.ValidateSource(false, error)) return false;
    const bool ok = insertion_aware
        ? CompareAligned(left, right, cancelled, batch_callback, progress_callback, error)
        : CompareEqual(left, right, cancelled, batch_callback, progress_callback, error);
    if (!ok || cancelled) return ok;
    if (!left.ValidateSource(false, error) || !right.ValidateSource(false, error)) return false;
    progress_callback(100); return true;
}
bool CompareFiles(const std::wstring& left_path, const std::wstring& right_path,
                  bool insertion_aware, const std::atomic_bool& cancelled,
                  const BatchCallback& batch_callback,
                  const ProgressCallback& progress_callback, std::wstring* error) {
    if (error) error->clear();
    BinaryDocument left, right;
    if (!left.Open(left_path, error) || !right.Open(right_path, error)) return false;
    return CompareDocuments(left, right, insertion_aware, cancelled, batch_callback, progress_callback, error);
}
} // namespace simple_editor::core


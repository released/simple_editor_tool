#include "binary_document.h"

#include <algorithm>
#include <filesystem>
#include <limits>

namespace simple_editor::core {
namespace {

std::wstring Win32Error(const wchar_t* context) {
    wchar_t* buffer = nullptr;
    const DWORD code = GetLastError();
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, 0, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    std::wstring result = context;
    result += L" (Win32 " + std::to_wstring(code) + L"): ";
    result += buffer != nullptr ? buffer : L"Unknown error";
    if (buffer != nullptr) {
        LocalFree(buffer);
    }
    return result;
}

bool SameState(const BY_HANDLE_FILE_INFORMATION& a, const BY_HANDLE_FILE_INFORMATION& b) {
    return a.dwVolumeSerialNumber == b.dwVolumeSerialNumber &&
           a.nFileIndexHigh == b.nFileIndexHigh && a.nFileIndexLow == b.nFileIndexLow &&
           a.nFileSizeHigh == b.nFileSizeHigh && a.nFileSizeLow == b.nFileSizeLow &&
           CompareFileTime(&a.ftLastWriteTime, &b.ftLastWriteTime) == 0;
}

bool InspectPath(const std::wstring& path, BY_HANDLE_FILE_INFORMATION* info,
                 bool* exists, std::wstring* error) {
    const HANDLE handle = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        const DWORD code = GetLastError(); *exists = false;
        if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND) return true;
        if (error) *error = Win32Error(L"Could not inspect destination");
        return false;
    }
    *exists = true;
    const bool ok = GetFileInformationByHandle(handle, info) != FALSE;
    if (!ok && error) *error = Win32Error(L"Could not inspect file state");
    CloseHandle(handle); return ok;
}

struct PendingFile {
    std::wstring path;
    HANDLE handle = INVALID_HANDLE_VALUE;
    bool committed = false;
    ~PendingFile() {
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
        if (!committed && !path.empty()) DeleteFileW(path.c_str());
    }
    void Close() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); handle = INVALID_HANDLE_VALUE; }
};

} // namespace

BinaryDocument::BinaryDocument() : file_(INVALID_HANDLE_VALUE), size_(0) {}

BinaryDocument::~BinaryDocument() {
    Close();
}

bool BinaryDocument::Open(const std::wstring& path, std::wstring* error) {
    BinaryDocument next;
    next.path_ = std::filesystem::absolute(path).wstring();
    if (!next.ReopenSource(error)) return false;
    if (!GetFileInformationByHandle(next.file_, &next.source_info_)) {
        if (error != nullptr) *error = Win32Error(L"Get file state failed");
        return false;
    }
    next.size_ = (static_cast<std::uint64_t>(next.source_info_.nFileSizeHigh) << 32) | next.source_info_.nFileSizeLow;
    if (next.size_ != 0) next.pieces_.push_back({Source::Original, 0, next.size_});
    next.RebuildIndex();
    Swap(next);
    return true;
}

void BinaryDocument::Swap(BinaryDocument& other) noexcept {
    std::swap(file_, other.file_); path_.swap(other.path_); std::swap(size_, other.size_);
    std::swap(source_info_, other.source_info_);
    added_.swap(other.added_); pieces_.swap(other.pieces_); starts_.swap(other.starts_);
    undo_.swap(other.undo_); redo_.swap(other.redo_);
}

std::unique_ptr<BinaryDocument> BinaryDocument::Snapshot(std::wstring* error, bool include_history) const {
    if (!ValidateSource(false, error)) return nullptr;
    auto copy = std::make_unique<BinaryDocument>();
    copy->file_ = ReOpenFile(file_, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE |
                             FILE_SHARE_DELETE, FILE_FLAG_RANDOM_ACCESS);
    if (copy->file_ == INVALID_HANDLE_VALUE) {
        if (error) *error = Win32Error(L"Snapshot failed");
        return nullptr;
    }
    copy->source_info_ = source_info_; copy->path_ = path_; copy->size_ = size_; copy->added_ = added_;
    copy->pieces_ = pieces_; copy->starts_ = starts_;
    if (include_history) { copy->undo_ = undo_; copy->redo_ = redo_; }
    if (!copy->ValidateSource(false, error)) return nullptr;
    return copy;
}

std::unique_ptr<BinaryDocument> BinaryDocument::SnapshotRange(std::uint64_t offset,std::uint64_t length,std::wstring* error) const {
    if (offset > size_ || length > size_ - offset) { if (error) *error = L"Snapshot range exceeds the document."; return nullptr; }
    auto copy = Snapshot(error); if (!copy) return nullptr;
    const auto begin = copy->SplitAt(offset), end = copy->SplitAt(offset + length);
    std::vector<Piece> selected(copy->pieces_.begin() + begin,copy->pieces_.begin() + end); copy->pieces_ = std::move(selected); copy->Normalize(); return copy;
}

void BinaryDocument::Close() {
    if (file_ != INVALID_HANDLE_VALUE) {
        CloseHandle(file_);
        file_ = INVALID_HANDLE_VALUE;
    }
    path_.clear();
    source_info_ = {};
    size_ = 0;
    pieces_.clear();
    starts_.clear();
    added_.clear();
    undo_.clear();
    redo_.clear();
}

bool BinaryDocument::IsOpen() const { return file_ != INVALID_HANDLE_VALUE; }
bool BinaryDocument::IsModified() const { return !undo_.empty(); }
std::uint64_t BinaryDocument::Size() const { return size_; }
const std::wstring& BinaryDocument::Path() const { return path_; }

bool BinaryDocument::ValidateSource(bool check_path, std::wstring* error) const {
    BY_HANDLE_FILE_INFORMATION current{};
    if (!IsOpen() || !GetFileInformationByHandle(file_, &current)) {
        if (error) *error = L"The source file is unavailable. Reload the file before continuing.";
        return false;
    }
    if (!SameState(source_info_, current)) {
        if (error) *error = L"The source file was modified externally. Edits are retained; reload before continuing.";
        return false;
    }
    if (check_path) {
        bool exists = false;
        if (!InspectPath(path_, &current, &exists, error)) return false;
        if (!exists || !SameState(source_info_, current)) {
            if (error) *error = L"The file path was replaced, deleted, or modified externally. Reload it, or Save As to a different file.";
            return false;
        }
    }
    return true;
}

bool BinaryDocument::ReopenSource(std::wstring* error) {
    file_ = CreateFileW(path_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE |
                        FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_ == INVALID_HANDLE_VALUE) {
        if (error != nullptr) *error = Win32Error(L"Open file failed");
        return false;
    }
    return true;
}

void BinaryDocument::RebuildIndex() {
    starts_.clear();
    std::uint64_t position = 0;
    for (const Piece& piece : pieces_) {
        starts_.push_back(position);
        position += piece.length;
    }
    size_ = position;
}

std::size_t BinaryDocument::FindPiece(std::uint64_t offset) const {
    const auto it = std::upper_bound(starts_.begin(), starts_.end(), offset);
    return static_cast<std::size_t>((it - starts_.begin()) - 1);
}

bool BinaryDocument::ReadOriginal(std::uint64_t offset, std::size_t length,
                                  std::uint8_t* output) const {
    LARGE_INTEGER position = {};
    position.QuadPart = static_cast<LONGLONG>(offset);
    if (!SetFilePointerEx(file_, position, nullptr, FILE_BEGIN)) return false;
    std::size_t completed = 0;
    while (completed < length) {
        const DWORD request = static_cast<DWORD>(
            (std::min)(length - completed, static_cast<std::size_t>(MAXDWORD)));
        DWORD read = 0;
        if (!ReadFile(file_, output + completed, request, &read, nullptr) || read == 0) return false;
        completed += read;
    }
    return true;
}

bool BinaryDocument::Read(std::uint64_t offset, std::size_t length,
                          std::vector<std::uint8_t>* data) const {
    if (data == nullptr) return false;
    data->clear();
    if (offset > size_ || !ValidateSource(false)) return false;
    const std::uint64_t available = size_ - offset;
    const std::size_t wanted = static_cast<std::size_t>((std::min<std::uint64_t>)(available, length));
    data->assign(wanted, 0);
    if (wanted == 0) return true;
    std::size_t piece_index = FindPiece(offset);
    std::uint64_t position = offset;
    std::size_t written = 0;
    while (written < wanted && piece_index < pieces_.size()) {
        const Piece& piece = pieces_[piece_index];
        const std::uint64_t relative = position - starts_[piece_index];
        const std::size_t count = static_cast<std::size_t>((std::min<std::uint64_t>)(
            piece.length - relative, wanted - written));
        if (piece.source == Source::Original) {
            if (!ReadOriginal(piece.start + relative, count, data->data() + written)) { data->clear(); return false; }
        } else {
            std::copy_n(added_.begin() + static_cast<std::ptrdiff_t>(piece.start + relative),
                        count, data->begin() + static_cast<std::ptrdiff_t>(written));
        }
        position += count;
        written += count;
        ++piece_index;
    }
    if (written != wanted || !ValidateSource(false)) { data->clear(); return false; }
    return true;
}

std::size_t BinaryDocument::SplitAt(std::uint64_t offset) {
    if (offset == size_) return pieces_.size();
    const std::size_t index = FindPiece(offset);
    const Piece piece = pieces_[index];
    const std::uint64_t relative = offset - starts_[index];
    if (relative == 0) return index;
    pieces_[index] = {piece.source, piece.start, relative};
    pieces_.insert(pieces_.begin() + static_cast<std::ptrdiff_t>(index + 1),
                   {piece.source, piece.start + relative, piece.length - relative});
    RebuildIndex();
    return index + 1;
}

void BinaryDocument::Normalize() {
    std::vector<Piece> normalized;
    for (const Piece& piece : pieces_) {
        if (piece.length == 0) continue;
        if (!normalized.empty() && normalized.back().source == piece.source &&
            normalized.back().start + normalized.back().length == piece.start) {
            normalized.back().length += piece.length;
        } else {
            normalized.push_back(piece);
        }
    }
    pieces_.swap(normalized);
    RebuildIndex();
}

bool BinaryDocument::ReplaceWithoutHistory(std::uint64_t offset, std::uint64_t remove_length,
                                           const std::vector<std::uint8_t>& data) {
    if (offset > size_ || remove_length > size_ - offset) return false;
    std::size_t start_index = SplitAt(offset);
    if (remove_length != 0) {
        const std::size_t end_index = SplitAt(offset + remove_length);
        start_index = SplitAt(offset);
        pieces_.erase(pieces_.begin() + static_cast<std::ptrdiff_t>(start_index),
                      pieces_.begin() + static_cast<std::ptrdiff_t>(end_index));
    }
    if (!data.empty()) {
        const std::uint64_t added_start = added_.size();
        added_.insert(added_.end(), data.begin(), data.end());
        pieces_.insert(pieces_.begin() + static_cast<std::ptrdiff_t>(start_index),
                       {Source::Added, added_start, data.size()});
    }
    Normalize();
    return true;
}

bool BinaryDocument::Replace(std::uint64_t offset, std::uint64_t remove_length,
                             const std::vector<std::uint8_t>& data) {
    std::vector<std::uint8_t> before;
    if (offset > size_ || remove_length > size_ - offset || !Read(offset,
        static_cast<std::size_t>(remove_length), &before)) return false;
    if (before == data) return true;
    if (!ReplaceWithoutHistory(offset, remove_length, data)) return false;
    undo_.push_back({{offset}, {offset}, std::move(before), data});
    redo_.clear();
    return true;
}

bool BinaryDocument::SetByte(std::uint64_t offset, std::uint8_t value) {
    return offset < size_ && Replace(offset, 1, {value});
}

bool BinaryDocument::Insert(std::uint64_t offset, const std::vector<std::uint8_t>& data) {
    return offset <= size_ && Replace(offset, 0, data);
}

bool BinaryDocument::Delete(std::uint64_t offset, std::uint64_t length) {
    return length == 0 || Replace(offset, length, {});
}

bool BinaryDocument::ReplaceBatch(const std::vector<std::uint64_t>& offsets,
                                  std::uint64_t length, const std::vector<std::uint8_t>& data) {
    std::uint64_t end = 0;
    for (const auto at : offsets) {
        if (at < end || at > size_ || length > size_ - at) return false;
        end = at + length;
    }
    std::vector<Piece> result;
    result.reserve(pieces_.size() + offsets.size() * 2);
    const std::uint64_t added_start = added_.size();
    std::size_t index = 0;
    auto append_range = [&](std::uint64_t begin, std::uint64_t finish) {
        while (begin < finish && index < pieces_.size()) {
            const auto& piece = pieces_[index];
            const std::uint64_t piece_end = starts_[index] + piece.length;
            if (piece_end <= begin) { ++index; continue; }
            const std::uint64_t count = (std::min)(finish, piece_end) - begin;
            result.push_back({piece.source, piece.start + begin - starts_[index], count});
            begin += count;
        }
    };
    std::uint64_t position = 0;
    for (const auto at : offsets) {
        append_range(position, at);
        if (!data.empty()) result.push_back({Source::Added, added_start, data.size()});
        position = at + length;
    }
    append_range(position, size_);
    added_.insert(added_.end(), data.begin(), data.end());
    pieces_.swap(result); Normalize(); return true;
}

bool BinaryDocument::ReplaceMatches(const std::vector<std::uint64_t>& offsets,
                                    const std::vector<std::uint8_t>& before,
                                    const std::vector<std::uint8_t>& after) {
    if (before.empty()) return false;
    if (offsets.empty() || before == after) return true;
    EditAction action{offsets, {}, before, after};
    std::uint64_t source_end = 0, target_end = 0;
    std::vector<std::uint8_t> actual;
    for (const auto at : offsets) {
        if (at < source_end || !Read(at, before.size(), &actual) || actual != before) return false;
        const auto target = target_end + (at - source_end);
        action.after_offsets.push_back(target);
        target_end = target + after.size(); source_end = at + before.size();
    }
    if (!ReplaceBatch(offsets, before.size(), after)) return false;
    undo_.push_back(std::move(action)); redo_.clear(); return true;
}

bool BinaryDocument::Undo(std::uint64_t* offset) {
    if (undo_.empty()) return false;
    const EditAction& current = undo_.back();
    if (!ReplaceBatch(current.after_offsets, current.after.size(), current.before)) return false;
    EditAction action = std::move(undo_.back());
    undo_.pop_back();
    if (offset != nullptr) *offset = action.offsets.front();
    redo_.push_back(std::move(action));
    return true;
}

bool BinaryDocument::Redo(std::uint64_t* offset) {
    if (redo_.empty()) return false;
    const EditAction& current = redo_.back();
    if (!ReplaceBatch(current.offsets, current.before.size(), current.after)) return false;
    EditAction action = std::move(redo_.back());
    redo_.pop_back();
    if (offset != nullptr) *offset = action.offsets.front();
    undo_.push_back(std::move(action));
    return true;
}

bool BinaryDocument::Save(const std::wstring& destination, std::wstring* error, const SaveControl& control) {
    if (error) error->clear();
    const auto cancelled = [&]() {
        if (!control.cancelled || !control.cancelled->load()) return false;
        if (error) *error = L"Save cancelled before replacement. The destination was not changed.";
        return true;
    };
    const auto progress = [&](unsigned int value) { if (control.progress) control.progress(value); };
    progress(0);
    if (cancelled()) return false;
    if (!ValidateSource(false, error)) return false;
    std::error_code path_error;
    const auto target_path = std::filesystem::absolute(destination.empty() ? path_ : destination, path_error).lexically_normal();
    if (path_error) { if (error) *error = L"Invalid save destination."; return false; }
    const std::wstring target = target_path.wstring();
    const bool same_path = _wcsicmp(target.c_str(), std::filesystem::path(path_).lexically_normal().c_str()) == 0;
    if (same_path && !ValidateSource(true, error)) return false;
    BY_HANDLE_FILE_INFORMATION destination_info{};
    bool destination_exists = false;
    if (!InspectPath(target, &destination_info, &destination_exists, error)) return false;
    wchar_t name[MAX_PATH]{};
    if (!GetTempFileNameW(target_path.parent_path().c_str(), L"set", 0, name)) {
        if (error) *error = Win32Error(L"Create temporary file failed");
        return false;
    }
    PendingFile pending; pending.path = name;
    pending.handle = CreateFileW(name, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (pending.handle == INVALID_HANDLE_VALUE) {
        if (error) *error = Win32Error(L"Open temporary file failed");
        return false;
    }
    constexpr std::size_t chunk_size = 1024 * 1024;
    std::vector<std::uint8_t> chunk;
    for (std::uint64_t position = 0; position < size_; position += chunk_size) {
        if (cancelled()) return false;
        const auto count = static_cast<std::size_t>((std::min<std::uint64_t>)(chunk_size, size_ - position));
        if (!Read(position, count, &chunk)) {
            if (!ValidateSource(same_path, error)) return false;
            if (error) *error = L"Could not read source data. The destination was not changed.";
            return false;
        }
        DWORD written = 0;
        if (!WriteFile(pending.handle, chunk.data(), static_cast<DWORD>(chunk.size()), &written, nullptr) || written != chunk.size()) {
            if (error) *error = Win32Error(L"Write temporary file failed");
            return false;
        }
        progress(static_cast<unsigned int>(static_cast<long double>(position + chunk.size()) * 98 / size_));
    }
    if (cancelled()) return false;
    if (!FlushFileBuffers(pending.handle)) {
        if (error) *error = Win32Error(L"Flush temporary file failed");
        return false;
    }
    pending.Close();
    BY_HANDLE_FILE_INFORMATION prepared_info{}; bool prepared_exists = false;
    if (!InspectPath(pending.path, &prepared_info, &prepared_exists, error) || !prepared_exists) return false;
    progress(99);
    if (cancelled()) return false;
    if (!ValidateSource(same_path, error)) return false;
    BY_HANDLE_FILE_INFORMATION current_destination{}; bool current_exists = false;
    if (!InspectPath(target, &current_destination, &current_exists, error)) return false;
    if (current_exists != destination_exists ||
        (current_exists && !SameState(current_destination, destination_info))) {
        if (error) *error = L"The destination changed while saving. Nothing was overwritten; retry or choose another path.";
        return false;
    }
    const std::wstring backup = pending.path + L".bak";
    if (GetFileAttributesW(backup.c_str()) != INVALID_FILE_ATTRIBUTES) {
        if (error) *error = L"A recovery filename already exists. Retry the save.";
        return false;
    }
    // Once native replacement begins, a completed commit takes precedence over cancellation.
    if (cancelled()) return false;
    const bool committed = destination_exists
        ? ReplaceFileW(target.c_str(), name, backup.c_str(), 0, nullptr, nullptr) != FALSE
        : MoveFileExW(name, target.c_str(), MOVEFILE_WRITE_THROUGH) != FALSE;
    if (!committed) {
        const std::wstring detail = Win32Error(L"Replace destination failed");
        if (GetFileAttributesW(backup.c_str()) != INVALID_FILE_ATTRIBUTES) {
            const bool target_missing = GetFileAttributesW(target.c_str()) == INVALID_FILE_ATTRIBUTES;
            if (!target_missing || !MoveFileExW(backup.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH)) {
                pending.committed = true;
                if (error) *error = detail + L" Recovery files were retained: " + backup + L" and " + pending.path;
                return false;
            }
        }
        if (error) *error = detail;
        return false;
    }
    pending.committed = true;
    BinaryDocument saved;
    if (!saved.Open(target, error) || !SameState(saved.source_info_, prepared_info)) {
        if (error) *error = L"The destination was written, but could not be reopened unchanged. Your edits are retained. Reload the file or recover the backup: " + backup;
        return false;
    }
    Swap(saved);
    if (destination_exists) DeleteFileW(backup.c_str());
    progress(100);
    return true;
}

} // namespace simple_editor::core


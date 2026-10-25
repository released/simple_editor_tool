#pragma once
#include "../core/diff_engine.h"
#include <mutex>
#include <condition_variable>
#include <deque>
#include <chrono>
// Each comparison owns its mailbox. The producer never touches native controls.
class DifferenceMailbox {
public:
    bool Push(simple_editor::core::DifferenceBatch batch,const std::atomic_bool& cancelled) {
        std::unique_lock<std::mutex> lock(mutex_); while (!stopped_ && !cancelled && batches_.size() >= 16) changed_.wait_for(lock,std::chrono::milliseconds(20));
        if (stopped_ || cancelled) return false; batches_.push_back(std::move(batch)); return true;
    }
    bool Pop(simple_editor::core::DifferenceBatch* batch) { std::lock_guard<std::mutex> lock(mutex_); if (batches_.empty()) return false; *batch = std::move(batches_.front()); batches_.pop_front(); changed_.notify_one(); return true; }
    void Stop() { std::lock_guard<std::mutex> lock(mutex_); stopped_ = true; batches_.clear(); changed_.notify_all(); }
private:
    std::mutex mutex_; std::condition_variable changed_; std::deque<simple_editor::core::DifferenceBatch> batches_; bool stopped_ = false;
};

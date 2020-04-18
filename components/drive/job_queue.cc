// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/job_queue.h"

#include <algorithm>

#include "base/logging.h"
#include "base/strings/stringprintf.h"

namespace drive {

JobQueue::Item::Item() : batchable(false), size(0) {
}

JobQueue::Item::Item(JobID id, bool batchable, uint64_t size)
    : id(id), batchable(batchable), size(size) {}

JobQueue::Item::~Item() {
}

JobQueue::JobQueue(size_t num_max_concurrent_jobs,
                   size_t num_priority_levels,
                   size_t num_max_batch_jobs,
                   size_t max_batch_size)
    : num_max_concurrent_jobs_(num_max_concurrent_jobs),
      queue_(num_priority_levels),
      num_max_batch_jobs_(num_max_batch_jobs),
      max_batch_size_(max_batch_size) {
}

JobQueue::~JobQueue() {
}

void JobQueue::PopForRun(int accepted_priority, std::vector<JobID>* jobs) {
  DCHECK_LT(accepted_priority, static_cast<int>(queue_.size()));
  jobs->clear();

  // Too many jobs are running already.
  if (running_.size() >= num_max_concurrent_jobs_)
    return;

  // Looks up the queue in the order of priority upto |accepted_priority|.
  uint64_t total_size = 0;
  bool batchable = true;
  for (int priority = 0; priority <= accepted_priority; ++priority) {
    while (!queue_[priority].empty()) {
      const auto& item = queue_[priority].front();
      total_size += item.size;
      batchable = batchable && item.batchable &&
                  jobs->size() < num_max_batch_jobs_ &&
                  total_size <= max_batch_size_;
      if (!(jobs->empty() || batchable))
        return;
      jobs->push_back(item.id);
      running_.insert(item.id);
      queue_[priority].pop_front();
    }
  }
}

void JobQueue::GetQueuedJobs(int priority, std::vector<JobID>* jobs) const {
  DCHECK_LT(priority, static_cast<int>(queue_.size()));
  jobs->clear();
  for (const Item& item : queue_[priority]) {
    jobs->push_back(item.id);
  }
}

void JobQueue::Push(JobID id, int priority, bool batchable, uint64_t size) {
  DCHECK_LT(priority, static_cast<int>(queue_.size()));
  queue_[priority].push_back(Item(id, batchable, size));
}

void JobQueue::MarkFinished(JobID id) {
  size_t num_erased = running_.erase(id);
  DCHECK_EQ(1U, num_erased);
}

std::string JobQueue::ToString() const {
  size_t pending = 0;
  for (size_t i = 0; i < queue_.size(); ++i)
    pending += queue_[i].size();
  return base::StringPrintf("pending: %d, running: %d",
                            static_cast<int>(pending),
                            static_cast<int>(running_.size()));
}

size_t JobQueue::GetNumberOfJobs() const {
  size_t count = running_.size();
  for (size_t i = 0; i < queue_.size(); ++i)
    count += queue_[i].size();
  return count;
}

void JobQueue::Remove(JobID id) {
  for (size_t i = 0; i < queue_.size(); ++i) {
    for (auto it = queue_[i].begin(); it != queue_[i].end(); ++it) {
      if (it->id == id) {
        queue_[i].erase(it);
        break;
      }
    }
  }
}

}  // namespace drive

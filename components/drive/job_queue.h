// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_JOB_QUEUE_H_
#define COMPONENTS_DRIVE_JOB_QUEUE_H_

#include <stddef.h>
#include <stdint.h>

#include <set>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "components/drive/job_list.h"

namespace drive {

// Priority queue for managing jobs in JobScheduler.
class JobQueue {
 public:
  // Creates a queue that allows |num_max_concurrent_jobs| concurrent job
  // execution and has |num_priority_levels| levels of priority.
  JobQueue(size_t num_max_concurrent_jobs,
           size_t num_priority_levels,
           size_t num_max_batch_jobs,
           size_t max_batch_size);
  ~JobQueue();

  // Pushes a job |id| of |priority|. The job with the smallest priority value
  // is popped first (lower values are higher priority). In the same priority,
  // the queue is "first-in first-out". If multiple jobs with |batchable| = true
  // are pushed continuously, there will be popped at the same time unless the
  // number of jobs exceeds |num_max_batch_jobs_| or the sum of |job_size|
  // exceeds or |max_batch_size_|.
  void Push(JobID id, int priority, bool batchable, uint64_t job_size);

  // Pops the first job which meets |accepted_priority| (i.e. the first job in
  // the queue with equal or higher priority (lower value)), and the limit of
  // concurrent job count is satisfied.
  //
  // For instance, if |accepted_priority| is 1, the first job with priority 0
  // (higher priority) in the queue is picked even if a job with priority 1 was
  // pushed earlier. If there is no job with priority 0, the first job with
  // priority 1 in the queue is picked.
  //
  // If the first found job and following jobs are batchable, these jobs are
  // popped out at the same time unless the total size of jobs exceeds
  // |max_batch_size_|.
  void PopForRun(int accepted_priority, std::vector<JobID>* jobs);

  // Gets queued jobs with the given priority.
  void GetQueuedJobs(int priority, std::vector<JobID>* jobs) const;

  // Marks a running job |id| as finished running. This decreases the count
  // of running parallel jobs and makes room for other jobs to be popped.
  void MarkFinished(JobID id);

  // Generates a string representing the internal state for logging.
  std::string ToString() const;

  // Gets the total number of jobs in the queue.
  size_t GetNumberOfJobs() const;

  // Removes the job from the queue.
  void Remove(JobID id);

 private:
  // JobID and additional properties that are needed to determine which tasks it
  // runs next.
  struct Item {
    Item();
    Item(JobID id, bool batchable, uint64_t size);
    ~Item();
    JobID id;
    bool batchable;
    uint64_t size;
  };

  const size_t num_max_concurrent_jobs_;
  std::vector<base::circular_deque<Item>> queue_;
  const size_t num_max_batch_jobs_;
  const size_t max_batch_size_;
  std::set<JobID> running_;

  DISALLOW_COPY_AND_ASSIGN(JobQueue);
};

}  // namespace drive

#endif  // COMPONENTS_DRIVE_JOB_QUEUE_H_

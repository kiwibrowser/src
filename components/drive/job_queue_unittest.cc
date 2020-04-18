// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/job_queue.h"

#include <stddef.h>

#include "testing/gtest/include/gtest/gtest.h"

namespace drive {

TEST(JobQueueTest, BasicJobQueueOperations) {
  const size_t kNumMaxConcurrentJobs = 3;
  const size_t kNumPriorityLevels = 2;
  const size_t kNumMaxBatchJob = 0;
  const size_t kMaxBatchSize = 0;
  enum {HIGH_PRIORITY, LOW_PRIORITY};

  // Create a queue. Number of jobs are initially zero.
  JobQueue queue(kNumMaxConcurrentJobs, kNumPriorityLevels, kNumMaxBatchJob,
                 kMaxBatchSize);
  EXPECT_EQ(0U, queue.GetNumberOfJobs());

  // Push 4 jobs.
  queue.Push(101, LOW_PRIORITY, false, 0);
  queue.Push(102, HIGH_PRIORITY, false, 0);
  queue.Push(103, LOW_PRIORITY, false, 0);
  queue.Push(104, HIGH_PRIORITY, false, 0);

  // High priority jobs should be popped first.
  std::vector<JobID> ids;
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(102, ids[0]);
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(104, ids[0]);

  // Then low priority jobs follow.
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(101, ids[0]);

  // The queue allows at most 3 parallel runs. So returns false here.
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(0u, ids.size());

  // No jobs finished yet, so the job count is four.
  EXPECT_EQ(4U, queue.GetNumberOfJobs());

  // Mark one job as finished.
  queue.MarkFinished(104);
  EXPECT_EQ(3U, queue.GetNumberOfJobs());

  // Then the next jobs can be popped.
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(103, ids[0]);

  // Push 1 more job.
  queue.Push(105, LOW_PRIORITY, false, 0);

  // Finish all 3 running jobs.
  queue.MarkFinished(101);
  queue.MarkFinished(102);
  queue.MarkFinished(103);
  EXPECT_EQ(1U, queue.GetNumberOfJobs());

  // The remaining jobs is of low priority, so under HIGH_PRIORITY context, it
  // cannot be popped for running.
  queue.PopForRun(HIGH_PRIORITY, &ids);
  ASSERT_EQ(0u, ids.size());

  // Under the low priority context, it is fine.
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(105, ids[0]);
}

TEST(JobQueueTest, JobQueueRemove) {
  const size_t kNumMaxConcurrentJobs = 3;
  const size_t kNumPriorityLevels = 2;
  const size_t kNumMaxBatchJob = 0;
  const size_t kMaxBatchSize = 0;
  enum {HIGH_PRIORITY, LOW_PRIORITY};

  // Create a queue. Number of jobs are initially zero.
  JobQueue queue(kNumMaxConcurrentJobs, kNumPriorityLevels, kNumMaxBatchJob,
                 kMaxBatchSize);
  EXPECT_EQ(0U, queue.GetNumberOfJobs());

  // Push 4 jobs.
  queue.Push(101, LOW_PRIORITY, false, 0);
  queue.Push(102, HIGH_PRIORITY, false, 0);
  queue.Push(103, LOW_PRIORITY, false, 0);
  queue.Push(104, HIGH_PRIORITY, false, 0);
  EXPECT_EQ(4U, queue.GetNumberOfJobs());

  // Remove 2.
  queue.Remove(101);
  queue.Remove(104);
  EXPECT_EQ(2U, queue.GetNumberOfJobs());

  // Pop the 2 jobs.
  std::vector<JobID> ids;
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(102, ids[0]);
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(103, ids[0]);
  queue.MarkFinished(102);
  queue.MarkFinished(103);

  // 0 job left.
  EXPECT_EQ(0U, queue.GetNumberOfJobs());
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(0u, ids.size());
}

TEST(JobQueueTest, BatchRequest) {
  const size_t kNumMaxConcurrentJobs = 1;
  const size_t kNumPriorityLevels = 2;
  const size_t kNumMaxBatchJob = 100;
  const size_t kMaxBatchSize = 5;
  enum { HIGH_PRIORITY, LOW_PRIORITY };

  // Create a queue. Number of jobs are initially zero.
  JobQueue queue(kNumMaxConcurrentJobs, kNumPriorityLevels, kNumMaxBatchJob,
                 kMaxBatchSize);
  EXPECT_EQ(0U, queue.GetNumberOfJobs());

  // Push 6 jobs.
  queue.Push(101, LOW_PRIORITY, true, 1);
  queue.Push(102, HIGH_PRIORITY, true, 1);
  queue.Push(103, LOW_PRIORITY, false, 1);
  queue.Push(104, HIGH_PRIORITY, true, 1);
  queue.Push(105, LOW_PRIORITY, true, 1);
  queue.Push(106, HIGH_PRIORITY, true, 10);

  EXPECT_EQ(6U, queue.GetNumberOfJobs());

  // Pop the 6 jobs.
  std::vector<JobID> ids;
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(2u, ids.size());
  EXPECT_EQ(102, ids[0]);
  EXPECT_EQ(104, ids[1]);
  queue.MarkFinished(102);
  queue.MarkFinished(104);
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(106, ids[0]);
  queue.MarkFinished(106);
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(101, ids[0]);
  queue.MarkFinished(101);
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(103, ids[0]);
  queue.MarkFinished(103);
  queue.PopForRun(LOW_PRIORITY, &ids);
  ASSERT_EQ(1u, ids.size());
  EXPECT_EQ(105, ids[0]);
  queue.MarkFinished(105);
  queue.PopForRun(LOW_PRIORITY, &ids);
  EXPECT_EQ(0u, ids.size());
}

TEST(JobQueueTest, BatchRequstNumMaxJobs) {
  const size_t kNumMaxConcurrentJobs = 1;
  const size_t kNumPriorityLevels = 2;
  const size_t kNumMaxBatchJob = 5;
  const size_t kMaxBatchSize = 100;
  enum { HIGH_PRIORITY, LOW_PRIORITY };

  // Create a queue. Number of jobs are initially zero.
  JobQueue queue(kNumMaxConcurrentJobs, kNumPriorityLevels, kNumMaxBatchJob,
                 kMaxBatchSize);
  EXPECT_EQ(0U, queue.GetNumberOfJobs());

  // Push 6 jobs.
  queue.Push(101, LOW_PRIORITY, true, 1);
  queue.Push(102, LOW_PRIORITY, true, 1);
  queue.Push(103, LOW_PRIORITY, true, 1);
  queue.Push(104, LOW_PRIORITY, true, 1);
  queue.Push(105, LOW_PRIORITY, true, 1);
  queue.Push(106, LOW_PRIORITY, true, 1);

  EXPECT_EQ(6U, queue.GetNumberOfJobs());

  // Pop the 5 of 6 jobs.
  std::vector<JobID> ids;
  queue.PopForRun(LOW_PRIORITY, &ids);
  EXPECT_EQ(5u, ids.size());
}

}  // namespace drive

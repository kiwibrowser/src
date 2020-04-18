// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/categorized_worker_pool.h"
#include "base/sequenced_task_runner.h"
#include "base/test/sequenced_task_runner_test_template.h"
#include "base/test/task_runner_test_template.h"
#include "base/threading/simple_thread.h"
#include "cc/test/task_graph_runner_test_template.h"

namespace base {
namespace {

// Number of threads spawned in tests.
const int kNumThreads = 4;

class CategorizedWorkerPoolTestDelegate {
 public:
  CategorizedWorkerPoolTestDelegate()
      : categorized_worker_pool_(new content::CategorizedWorkerPool()) {}

  void StartTaskRunner() { categorized_worker_pool_->Start(kNumThreads); }

  scoped_refptr<content::CategorizedWorkerPool> GetTaskRunner() {
    return categorized_worker_pool_;
  }

  void StopTaskRunner() { categorized_worker_pool_->FlushForTesting(); }

  ~CategorizedWorkerPoolTestDelegate() { categorized_worker_pool_->Shutdown(); }

 private:
  scoped_refptr<content::CategorizedWorkerPool> categorized_worker_pool_;
};

INSTANTIATE_TYPED_TEST_CASE_P(CategorizedWorkerPool,
                              TaskRunnerTest,
                              CategorizedWorkerPoolTestDelegate);

class CategorizedWorkerPoolSequencedTestDelegate {
 public:
  CategorizedWorkerPoolSequencedTestDelegate()
      : categorized_worker_pool_(new content::CategorizedWorkerPool()) {}

  void StartTaskRunner() { categorized_worker_pool_->Start(kNumThreads); }

  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() {
    return categorized_worker_pool_->CreateSequencedTaskRunner();
  }

  void StopTaskRunner() { categorized_worker_pool_->FlushForTesting(); }

  ~CategorizedWorkerPoolSequencedTestDelegate() {
    categorized_worker_pool_->Shutdown();
  }

 private:
  scoped_refptr<content::CategorizedWorkerPool> categorized_worker_pool_;
};

INSTANTIATE_TYPED_TEST_CASE_P(CategorizedWorkerPool,
                              SequencedTaskRunnerTest,
                              CategorizedWorkerPoolSequencedTestDelegate);

}  // namespace
}  // namespace base

namespace cc {
namespace {

template <int NumThreads>
class CategorizedWorkerPoolTaskGraphRunnerTestDelegate {
 public:
  CategorizedWorkerPoolTaskGraphRunnerTestDelegate()
      : categorized_worker_pool_(new content::CategorizedWorkerPool()) {}

  void StartTaskGraphRunner() { categorized_worker_pool_->Start(NumThreads); }

  cc::TaskGraphRunner* GetTaskGraphRunner() {
    return categorized_worker_pool_->GetTaskGraphRunner();
  }

  void StopTaskGraphRunner() { categorized_worker_pool_->FlushForTesting(); }

  ~CategorizedWorkerPoolTaskGraphRunnerTestDelegate() {
    categorized_worker_pool_->Shutdown();
  }

 private:
  scoped_refptr<content::CategorizedWorkerPool> categorized_worker_pool_;
};

// Multithreaded tests.
INSTANTIATE_TYPED_TEST_CASE_P(
    CategorizedWorkerPool_1_Threads,
    TaskGraphRunnerTest,
    CategorizedWorkerPoolTaskGraphRunnerTestDelegate<1>);
INSTANTIATE_TYPED_TEST_CASE_P(
    CategorizedWorkerPool_2_Threads,
    TaskGraphRunnerTest,
    CategorizedWorkerPoolTaskGraphRunnerTestDelegate<2>);
INSTANTIATE_TYPED_TEST_CASE_P(
    CategorizedWorkerPool_3_Threads,
    TaskGraphRunnerTest,
    CategorizedWorkerPoolTaskGraphRunnerTestDelegate<3>);
INSTANTIATE_TYPED_TEST_CASE_P(
    CategorizedWorkerPool_4_Threads,
    TaskGraphRunnerTest,
    CategorizedWorkerPoolTaskGraphRunnerTestDelegate<4>);
INSTANTIATE_TYPED_TEST_CASE_P(
    CategorizedWorkerPool_5_Threads,
    TaskGraphRunnerTest,
    CategorizedWorkerPoolTaskGraphRunnerTestDelegate<5>);

// Single threaded tests.
INSTANTIATE_TYPED_TEST_CASE_P(
    CategorizedWorkerPool,
    SingleThreadTaskGraphRunnerTest,
    CategorizedWorkerPoolTaskGraphRunnerTestDelegate<1>);

}  // namespace
}  // namespace cc

// Copyright (c) 2013 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Simple thread pool class

#ifndef NATIVE_CLIENT_TESTS_BENCHMARK_THREAD_POOL_H_
#define NATIVE_CLIENT_TESTS_BENCHMARK_THREAD_POOL_H_

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

namespace sdk_util {

// typdef helper for work function
typedef void (*WorkFunction)(int task_index, void* data);

// ThreadPool is a class to manage num_threads and assign
// them num_tasks of work at a time. Each call
// to Dispatch(..) will block until all tasks complete.
// If 0 is passed in for num_threads, all tasks will be
// issued on the dispatch thread.

class ThreadPool {
 public:
  void Dispatch(int num_tasks, WorkFunction work, void* data);
  explicit ThreadPool(int num_threads);
  ~ThreadPool();
 private:
  int DecCounter();
  void Setup(int counter, WorkFunction work, void* data);
  void DispatchMany(int num_tasks, WorkFunction work, void* data);
  void DispatchHere(int num_tasks, WorkFunction work, void* data);
  void WorkLoop();
  static void* WorkerThreadEntry(void* data);
  void PostExitAndJoinAll();
  pthread_t* threads_;
  int32_t counter_;
  const int num_threads_;
  bool exiting_;
  void* user_data_;
  WorkFunction user_work_function_;
  sem_t work_sem_;
  sem_t done_sem_;
};

}  // namespace sdk_util

#endif  // NATIVE_CLIENT_TESTS_BENCHMARK_THREAD_POOL_H_

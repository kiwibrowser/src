// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_THREAD_PROFILER_H_
#define CHROME_COMMON_THREAD_PROFILER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/profiler/stack_sampling_profiler.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "components/metrics/call_stack_profile_params.h"

namespace service_manager {
class Connector;
}

// PeriodicSamplingScheduler repeatedly schedules periodic sampling of the
// thread through calls to GetTimeToNextCollection(). This class is exposed
// to allow testing.
class PeriodicSamplingScheduler {
 public:
  PeriodicSamplingScheduler(base::TimeDelta sampling_duration,
                            double fraction_of_execution_time_to_sample,
                            base::TimeTicks start_time);
  virtual ~PeriodicSamplingScheduler();

  // Returns the amount of time between now and the next collection.
  base::TimeDelta GetTimeToNextCollection();

 protected:
  // Virtual to provide seams for test use.
  virtual double RandDouble() const;
  virtual base::TimeTicks Now() const;

 private:
  const base::TimeDelta period_duration_;
  const base::TimeDelta sampling_duration_;
  base::TimeTicks period_start_time_;

  DISALLOW_COPY_AND_ASSIGN(PeriodicSamplingScheduler);
};

// ThreadProfiler performs startup and periodic profiling of Chrome
// threads.
class ThreadProfiler {
 public:
  ~ThreadProfiler();

  // Creates a profiler for a main thread and immediately starts it. This
  // function should only be used when profiling the main thread of a
  // process. The returned profiler must be destroyed prior to thread exit to
  // stop the profiling. SetMainThreadTaskRunner() should be called after the
  // message loop has been started on the thread.
  static std::unique_ptr<ThreadProfiler> CreateAndStartOnMainThread();

  // Sets the task runner when profiling on the main thread. This occurs in a
  // separate call from CreateAndStartOnMainThread so that startup profiling can
  // occur prior to message loop start.
  void SetMainThreadTaskRunner(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  // Creates a profiler for a child thread and immediately starts it. This
  // should be called from a task posted on the child thread immediately after
  // thread start. The thread will be profiled until exit.
  static void StartOnChildThread(
      metrics::CallStackProfileParams::Thread thread);

  // This function must be called within child processes to supply the Service
  // Manager's connector, to bind the interface through which profiles are sent
  // back to the browser process.
  //
  // Note that the metrics::CallStackProfileCollector interface also must be
  // exposed to the child process, and metrics::mojom::CallStackProfileCollector
  // declared in chrome_content_browser_manifest_overlay.json, for the binding
  // to succeed.
  static void SetServiceManagerConnectorForChildProcess(
      service_manager::Connector* connector);

 private:
  // Creates the profiler. The task runner will be supplied for child threads
  // but not for main threads.
  ThreadProfiler(
      metrics::CallStackProfileParams::Thread thread,
      scoped_refptr<base::SingleThreadTaskRunner> owning_thread_task_runner =
          scoped_refptr<base::SingleThreadTaskRunner>());

  // Gets the completed callback for the ultimate receiver of the profiles.
  base::StackSamplingProfiler::CompletedCallback GetReceiverCallback(
      const metrics::CallStackProfileParams& profile_params);

  // Receives |profiles| from the StackSamplingProfiler and forwards them on to
  // the original |receiver_callback|.  Note that we must obtain and bind the
  // original receiver callback prior to the start of collection because the
  // collection start time is currently recorded when obtaining the callback in
  // some collection scenarios. The implementation contains a TODO to fix this.
  static void ReceiveStartupProfiles(
      const base::StackSamplingProfiler::CompletedCallback& receiver_callback,
      base::StackSamplingProfiler::CallStackProfiles profiles);
  static void ReceivePeriodicProfiles(
      const base::StackSamplingProfiler::CompletedCallback& receiver_callback,
      scoped_refptr<base::SingleThreadTaskRunner> owning_thread_task_runner,
      base::WeakPtr<ThreadProfiler> thread_profiler,
      base::StackSamplingProfiler::CallStackProfiles profiles);

  // Posts a delayed task to start the next periodic sampling collection.
  void ScheduleNextPeriodicCollection();

  // Creates a new periodic profiler and initiates a collection with it.
  void StartPeriodicSamplingCollection();

  scoped_refptr<base::SingleThreadTaskRunner> owning_thread_task_runner_;

  // TODO(wittman): Make const after cleaning up the existing continuous
  // collection support.
  metrics::CallStackProfileParams periodic_profile_params_;

  std::unique_ptr<base::StackSamplingProfiler> startup_profiler_;

  std::unique_ptr<base::StackSamplingProfiler> periodic_profiler_;
  std::unique_ptr<PeriodicSamplingScheduler> periodic_sampling_scheduler_;

  THREAD_CHECKER(thread_checker_);
  base::WeakPtrFactory<ThreadProfiler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThreadProfiler);
};

#endif  // CHROME_COMMON_THREAD_PROFILER_H_

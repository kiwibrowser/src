// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/thread_profiler.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/rand_util.h"
#include "base/threading/platform_thread.h"
#include "base/threading/sequence_local_storage_slot.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/common/stack_sampling_configuration.h"
#include "components/metrics/call_stack_profile_metrics_provider.h"
#include "components/metrics/call_stack_profile_params.h"
#include "components/metrics/child_call_stack_profile_collector.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_names.mojom.h"
#include "services/service_manager/embedder/switches.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

// Only used by child processes.
base::LazyInstance<metrics::ChildCallStackProfileCollector>::Leaky
    g_child_call_stack_profile_collector = LAZY_INSTANCE_INITIALIZER;

// The profiler object is stored in a SequenceLocalStorageSlot on child threads
// to give it the same lifetime as the threads.
base::LazyInstance<
    base::SequenceLocalStorageSlot<std::unique_ptr<ThreadProfiler>>>::Leaky
    g_child_thread_profiler_sequence_local_storage = LAZY_INSTANCE_INITIALIZER;

// Run continuous profiling 2% of the time.
constexpr const double kFractionOfExecutionTimeToSample = 0.02;

base::StackSamplingProfiler::SamplingParams GetSamplingParams() {
  base::StackSamplingProfiler::SamplingParams params;
  params.initial_delay = base::TimeDelta::FromMilliseconds(0);
  params.bursts = 1;
  params.samples_per_burst = 300;
  params.sampling_interval = base::TimeDelta::FromMilliseconds(100);
  return params;
}

metrics::CallStackProfileParams::Process GetProcess() {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line->GetSwitchValueASCII(switches::kProcessType);
  if (process_type.empty())
    return metrics::CallStackProfileParams::BROWSER_PROCESS;
  if (process_type == switches::kRendererProcess)
    return metrics::CallStackProfileParams::RENDERER_PROCESS;
  if (process_type == switches::kGpuProcess)
    return metrics::CallStackProfileParams::GPU_PROCESS;
  if (process_type == switches::kUtilityProcess)
    return metrics::CallStackProfileParams::UTILITY_PROCESS;
  if (process_type == service_manager::switches::kZygoteProcess)
    return metrics::CallStackProfileParams::ZYGOTE_PROCESS;
  if (process_type == switches::kPpapiPluginProcess)
    return metrics::CallStackProfileParams::PPAPI_PLUGIN_PROCESS;
  if (process_type == switches::kPpapiBrokerProcess)
    return metrics::CallStackProfileParams::PPAPI_BROKER_PROCESS;
  return metrics::CallStackProfileParams::UNKNOWN_PROCESS;
}

}  // namespace

// The scheduler works by splitting execution time into repeated periods such
// that the time to take one collection represents
// |fraction_of_execution_time_to_sample| of the period, and the time not spent
// sampling represents 1 - |fraction_of_execution_time_to_sample| of the period.
// The collection start time is chosen randomly within each period such that the
// entire collection is contained within the period.
//
// The kFractionOfExecutionTimeToSample and SamplingParams settings at the top
// of the file specify fraction = 0.02 and sampling period = 1 sample / .1s
// sampling interval * 300 samples = 30s. The period length works out to
// 30s/0.02 = 1500s = 25m. So every 25 minutes a random 30 second continuous
// interval will be picked to sample.
PeriodicSamplingScheduler::PeriodicSamplingScheduler(
    base::TimeDelta sampling_duration,
    double fraction_of_execution_time_to_sample,
    base::TimeTicks start_time)
    : period_duration_(
          base::TimeDelta::FromSecondsD(sampling_duration.InSecondsF() /
                                        fraction_of_execution_time_to_sample)),
      sampling_duration_(sampling_duration),
      period_start_time_(start_time) {
  DCHECK(sampling_duration_ <= period_duration_);
}

PeriodicSamplingScheduler::~PeriodicSamplingScheduler() = default;

base::TimeDelta PeriodicSamplingScheduler::GetTimeToNextCollection() {
  double sampling_offset_seconds =
      (period_duration_ - sampling_duration_).InSecondsF() * RandDouble();
  base::TimeTicks next_collection_time =
      period_start_time_ +
      base::TimeDelta::FromSecondsD(sampling_offset_seconds);
  period_start_time_ += period_duration_;
  return next_collection_time - Now();
}

double PeriodicSamplingScheduler::RandDouble() const {
  return base::RandDouble();
}

base::TimeTicks PeriodicSamplingScheduler::Now() const {
  return base::TimeTicks::Now();
}

ThreadProfiler::~ThreadProfiler() {}

// static
std::unique_ptr<ThreadProfiler> ThreadProfiler::CreateAndStartOnMainThread() {
  return std::unique_ptr<ThreadProfiler>(
      new ThreadProfiler(metrics::CallStackProfileParams::MAIN_THREAD));
}

void ThreadProfiler::SetMainThreadTaskRunner(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  if (!StackSamplingConfiguration::Get()->IsProfilerEnabledForCurrentProcess())
    return;

  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // This should only be called if the task runner wasn't provided in the
  // constructor.
  DCHECK(!owning_thread_task_runner_);
  owning_thread_task_runner_ = task_runner;
  ScheduleNextPeriodicCollection();
}

// static
void ThreadProfiler::StartOnChildThread(
    metrics::CallStackProfileParams::Thread thread) {
  if (!StackSamplingConfiguration::Get()->IsProfilerEnabledForCurrentProcess())
    return;

  auto profiler = std::unique_ptr<ThreadProfiler>(
      new ThreadProfiler(thread, base::ThreadTaskRunnerHandle::Get()));
  g_child_thread_profiler_sequence_local_storage.Get().Set(std::move(profiler));
}

void ThreadProfiler::SetServiceManagerConnectorForChildProcess(
    service_manager::Connector* connector) {
  if (!StackSamplingConfiguration::Get()->IsProfilerEnabledForCurrentProcess())
    return;

  DCHECK_NE(metrics::CallStackProfileParams::BROWSER_PROCESS, GetProcess());

  metrics::mojom::CallStackProfileCollectorPtr browser_interface;
  connector->BindInterface(content::mojom::kBrowserServiceName,
                           &browser_interface);
  g_child_call_stack_profile_collector.Get().SetParentProfileCollector(
      std::move(browser_interface));
}

// ThreadProfiler implementation synopsis:
//
// On creation, the profiler creates and starts the startup
// StackSamplingProfiler, and configures the PeriodicSamplingScheduler such that
// it starts scheduling from the time the startup profiling will be complete.
// When a message loop is available (either in the constructor, or via
// SetMainThreadTaskRunner) a task is posted to start the first periodic
// collection at the initial scheduled collection time.
//
// When the periodic collection task executes, it creates and starts a new
// periodic profiler and configures it to call ReceivePeriodicProfiles as its
// completion callback. ReceivePeriodicProfiles is called on the profiler thread
// and passes the profiles along to their ultimate destination, then schedules a
// task on the original thread to schedule another periodic collection. When the
// task runs, it posts a new task to start another periodic collection at the
// next scheduled collection time.
//
// The process in previous paragraph continues until the ThreadProfiler is
// destroyed prior to thread exit.
ThreadProfiler::ThreadProfiler(
    metrics::CallStackProfileParams::Thread thread,
    scoped_refptr<base::SingleThreadTaskRunner> owning_thread_task_runner)
    : owning_thread_task_runner_(owning_thread_task_runner),
      periodic_profile_params_(
          GetProcess(),
          thread,
          metrics::CallStackProfileParams::PERIODIC_COLLECTION,
          metrics::CallStackProfileParams::MAY_SHUFFLE),
      weak_factory_(this) {
  if (!StackSamplingConfiguration::Get()->IsProfilerEnabledForCurrentProcess())
    return;

  startup_profiler_ = std::make_unique<base::StackSamplingProfiler>(
      base::PlatformThread::CurrentId(), GetSamplingParams(),
      BindRepeating(&ThreadProfiler::ReceiveStartupProfiles,
                    GetReceiverCallback(metrics::CallStackProfileParams(
                        GetProcess(), thread,
                        metrics::CallStackProfileParams::PROCESS_STARTUP,
                        metrics::CallStackProfileParams::MAY_SHUFFLE))));
  startup_profiler_->Start();

  const base::StackSamplingProfiler::SamplingParams& sampling_params =
      GetSamplingParams();

  // Estimated time at which the startup profiling will be completed. It's OK if
  // this doesn't exactly coincide with the end of the startup profiling, since
  // there's no harm in having a brief overlap of startup and periodic
  // profiling.
  base::TimeTicks startup_profiling_completion_time =
      base::TimeTicks::Now() +
      sampling_params.samples_per_burst * sampling_params.sampling_interval;

  periodic_sampling_scheduler_ = std::make_unique<PeriodicSamplingScheduler>(
      sampling_params.samples_per_burst * sampling_params.sampling_interval,
      kFractionOfExecutionTimeToSample, startup_profiling_completion_time);

  if (owning_thread_task_runner_)
    ScheduleNextPeriodicCollection();
}

base::StackSamplingProfiler::CompletedCallback
ThreadProfiler::GetReceiverCallback(
    const metrics::CallStackProfileParams& profile_params) {
  // TODO(wittman): Simplify the approach to getting the profiler callback
  // across CallStackProfileMetricsProvider and
  // ChildCallStackProfileCollector. Ultimately both should expose functions
  // like
  //
  //   void ReceiveCompletedProfiles(
  //       const metrics::CallStackProfileParams& profile_params,
  //       base::TimeTicks profile_start_time,
  //       base::StackSamplingProfiler::CallStackProfiles profiles);
  //
  // and this function should bind the passed profile_params and
  // base::TimeTicks::Now() to those functions.
  base::TimeTicks profile_start_time = base::TimeTicks::Now();
  if (GetProcess() == metrics::CallStackProfileParams::BROWSER_PROCESS) {
    return metrics::CallStackProfileMetricsProvider::
        GetProfilerCallbackForBrowserProcess(profile_params);
  }
  return g_child_call_stack_profile_collector.Get()
      .ChildCallStackProfileCollector::GetProfilerCallback(profile_params,
                                                           profile_start_time);
}

// static
void ThreadProfiler::ReceiveStartupProfiles(
    const base::StackSamplingProfiler::CompletedCallback& receiver_callback,
    base::StackSamplingProfiler::CallStackProfiles profiles) {
  receiver_callback.Run(std::move(profiles));
}

// static
void ThreadProfiler::ReceivePeriodicProfiles(
    const base::StackSamplingProfiler::CompletedCallback& receiver_callback,
    scoped_refptr<base::SingleThreadTaskRunner> owning_thread_task_runner,
    base::WeakPtr<ThreadProfiler> thread_profiler,
    base::StackSamplingProfiler::CallStackProfiles profiles) {
  receiver_callback.Run(std::move(profiles));
  owning_thread_task_runner->PostTask(
      FROM_HERE, base::BindOnce(&ThreadProfiler::ScheduleNextPeriodicCollection,
                                thread_profiler));
}

void ThreadProfiler::ScheduleNextPeriodicCollection() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  owning_thread_task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ThreadProfiler::StartPeriodicSamplingCollection,
                     weak_factory_.GetWeakPtr()),
      periodic_sampling_scheduler_->GetTimeToNextCollection());
}

void ThreadProfiler::StartPeriodicSamplingCollection() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // NB: Destroys the previous profiler as side effect.
  periodic_profiler_ = std::make_unique<base::StackSamplingProfiler>(
      base::PlatformThread::CurrentId(), GetSamplingParams(),
      BindRepeating(&ThreadProfiler::ReceivePeriodicProfiles,
                    GetReceiverCallback(periodic_profile_params_),
                    owning_thread_task_runner_, weak_factory_.GetWeakPtr()));
  periodic_profiler_->Start();
}

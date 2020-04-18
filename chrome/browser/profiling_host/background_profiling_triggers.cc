// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiling_host/background_profiling_triggers.h"

#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiling_host/profiling_process_host.h"
#include "components/heap_profiling/supervisor.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/process_type.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation.h"

namespace heap_profiling {

namespace {
// Check memory usage every hour. Trigger slow report if needed.
const int kRepeatingCheckMemoryDelayInHours = 1;
const int kThrottledReportRepeatingCheckMemoryDelayInHours = 12;

#if defined(OS_ANDROID)
const size_t kBrowserProcessMallocTriggerKb = 100 * 1024;    // 100 MB
const size_t kGPUProcessMallocTriggerKb = 40 * 1024;         // 40 MB
const size_t kRendererProcessMallocTriggerKb = 125 * 1024;   // 125 MB
#else
const size_t kBrowserProcessMallocTriggerKb = 400 * 1024;    // 400 MB
const size_t kGPUProcessMallocTriggerKb = 400 * 1024;        // 400 MB
const size_t kRendererProcessMallocTriggerKb = 500 * 1024;   // 500 MB
#endif  // OS_ANDROID

int GetContentProcessType(
    const memory_instrumentation::mojom::ProcessType& type) {
  using memory_instrumentation::mojom::ProcessType;

  switch (type) {
    case ProcessType::BROWSER:
      return content::ProcessType::PROCESS_TYPE_BROWSER;

    case ProcessType::RENDERER:
      return content::ProcessType::PROCESS_TYPE_RENDERER;

    case ProcessType::GPU:
      return content::ProcessType::PROCESS_TYPE_GPU;

    case ProcessType::UTILITY:
      return content::ProcessType::PROCESS_TYPE_UTILITY;

    case ProcessType::PLUGIN:
      return content::ProcessType::PROCESS_TYPE_PLUGIN_DEPRECATED;

    case ProcessType::OTHER:
      return content::ProcessType::PROCESS_TYPE_UNKNOWN;
  }

  NOTREACHED();
  return content::ProcessType::PROCESS_TYPE_UNKNOWN;
}

}  // namespace

BackgroundProfilingTriggers::BackgroundProfilingTriggers(
    ProfilingProcessHost* host)
    : host_(host), weak_ptr_factory_(this) {
  DCHECK(host_);
}

BackgroundProfilingTriggers::~BackgroundProfilingTriggers() {}

void BackgroundProfilingTriggers::StartTimer() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  // Register a repeating timer to check memory usage periodically.
  timer_.Start(FROM_HERE,
               base::TimeDelta::FromHours(kRepeatingCheckMemoryDelayInHours),
               base::BindRepeating(
                   &BackgroundProfilingTriggers::PerformMemoryUsageChecks,
                   weak_ptr_factory_.GetWeakPtr()));
}

bool BackgroundProfilingTriggers::IsAllowedToUpload() const {
  return ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled();
}

bool BackgroundProfilingTriggers::IsOverTriggerThreshold(
    int content_process_type,
    uint32_t private_footprint_kb) {
  switch (content_process_type) {
    case content::ProcessType::PROCESS_TYPE_BROWSER:
      return private_footprint_kb > kBrowserProcessMallocTriggerKb;

    case content::ProcessType::PROCESS_TYPE_GPU:
      return private_footprint_kb > kGPUProcessMallocTriggerKb;

    case content::ProcessType::PROCESS_TYPE_RENDERER:
      return private_footprint_kb > kRendererProcessMallocTriggerKb;

    default:
      return false;
  }
}

void BackgroundProfilingTriggers::PerformMemoryUsageChecks() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  if (!IsAllowedToUpload()) {
    return;
  }

  auto callback = base::BindOnce(
      [](base::WeakPtr<BackgroundProfilingTriggers> weak_ptr,
         std::vector<base::ProcessId> result) {
        memory_instrumentation::MemoryInstrumentation::GetInstance()
            ->RequestGlobalDump(
                base::Bind(&BackgroundProfilingTriggers::OnReceivedMemoryDump,
                           std::move(weak_ptr), std::move(result)));
      },
      weak_ptr_factory_.GetWeakPtr());
  Supervisor::GetInstance()->GetProfiledPids(std::move(callback));
}

void BackgroundProfilingTriggers::OnReceivedMemoryDump(
    std::vector<base::ProcessId> profiled_pids,
    bool success,
    std::unique_ptr<memory_instrumentation::GlobalMemoryDump> dump) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  if (!success) {
    return;
  }

  bool should_send_report = false;
  for (const auto& proc : dump->process_dumps()) {
    if (std::find(profiled_pids.begin(), profiled_pids.end(), proc.pid()) ==
        profiled_pids.end())
      continue;
    if (IsOverTriggerThreshold(GetContentProcessType(proc.process_type()),
                               proc.os_dump().private_footprint_kb)) {
      should_send_report = true;
      break;
    }
  }

  if (should_send_report) {
    TriggerMemoryReport();

    // If a report was sent, throttle the memory data collection rate to
    // kThrottledReportRepeatingCheckMemoryDelayInHours to avoid sending too
    // many reports from a known problematic client.
    timer_.Start(FROM_HERE,
                 base::TimeDelta::FromHours(
                     kThrottledReportRepeatingCheckMemoryDelayInHours),
                 base::BindRepeating(
                     &BackgroundProfilingTriggers::PerformMemoryUsageChecks,
                     weak_ptr_factory_.GetWeakPtr()));
  }
}

void BackgroundProfilingTriggers::TriggerMemoryReport() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  host_->RequestProcessReport("MEMLOG_BACKGROUND_TRIGGER");
}

}  // namespace heap_profiling

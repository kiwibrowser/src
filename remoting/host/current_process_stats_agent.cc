// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/current_process_stats_agent.h"

#include "base/process/process_metrics.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>  // This include must come first.

#include <psapi.h>
#endif

namespace remoting {

CurrentProcessStatsAgent::CurrentProcessStatsAgent(
    const std::string& process_name)
    : process_name_(process_name),
      metrics_(base::ProcessMetrics::CreateCurrentProcessMetrics()) {}

CurrentProcessStatsAgent::~CurrentProcessStatsAgent() = default;

protocol::ProcessResourceUsage CurrentProcessStatsAgent::GetResourceUsage() {
  protocol::ProcessResourceUsage current;
  current.set_process_name(process_name_);
  current.set_processor_usage(metrics_->GetPlatformIndependentCPUUsage());

// The concepts of "Page File" and "Working Set" are only well defined on
// Windows.
// TODO: Currently, this code is only run on Windows. In the future, if/when
// this code runs on other platforms, consistent memory metrics should be
// obtained from the memory-infra service.
#if defined(OS_WIN)
  PROCESS_MEMORY_COUNTERS pmc;
  if (::GetProcessMemoryInfo(::GetCurrentProcess(), &pmc, sizeof(pmc))) {
    current.set_pagefile_size(pmc.PagefileUsage);
    current.set_working_set_size(pmc.WorkingSetSize);
  }
#endif

  return current;
}

}  // namespace remoting

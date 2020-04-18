// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/process_stats_sender.h"

#include <utility>

#include "base/location.h"
#include "base/logging.h"
#include "remoting/host/process_stats_agent.h"

namespace remoting {

namespace {

bool IsProcessResourceUsageValid(const protocol::ProcessResourceUsage& usage) {
  return usage.has_process_name() && usage.has_processor_usage() &&
         usage.has_working_set_size() && usage.has_pagefile_size();
}

}  // namespace

ProcessStatsSender::ProcessStatsSender(
    protocol::ProcessStatsStub* host_stats_stub,
    base::TimeDelta interval,
    std::initializer_list<ProcessStatsAgent*> agents)
    : host_stats_stub_(host_stats_stub),
      agents_(agents),
      interval_(interval),
      thread_checker_() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(host_stats_stub_);
  DCHECK(interval > base::TimeDelta());
  DCHECK(!agents_.empty());

  timer_.Start(FROM_HERE, interval, this, &ProcessStatsSender::ReportUsage);
}

ProcessStatsSender::~ProcessStatsSender() {
  DCHECK(thread_checker_.CalledOnValidThread());
  timer_.Stop();
}

base::TimeDelta ProcessStatsSender::interval() const {
  return interval_;
}

void ProcessStatsSender::ReportUsage() {
  DCHECK(thread_checker_.CalledOnValidThread());

  protocol::AggregatedProcessResourceUsage aggregated;
  for (auto* const agent : agents_) {
    DCHECK(agent);
    protocol::ProcessResourceUsage usage = agent->GetResourceUsage();
    if (IsProcessResourceUsageValid(usage)) {
      *aggregated.add_usages() = usage;
    } else {
      LOG(ERROR) << "Invalid ProcessResourceUsage "
                 << usage.process_name()
                 << " received.";
    }
  }

  host_stats_stub_->OnProcessStats(aggregated);
}

}  // namespace remoting

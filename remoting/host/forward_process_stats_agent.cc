// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/forward_process_stats_agent.h"

#include "base/logging.h"

namespace remoting {

ForwardProcessStatsAgent::ForwardProcessStatsAgent() = default;
ForwardProcessStatsAgent::~ForwardProcessStatsAgent() = default;

protocol::ProcessResourceUsage ForwardProcessStatsAgent::GetResourceUsage() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return usage_;
}

void ForwardProcessStatsAgent::OnProcessStats(
    const protocol::ProcessResourceUsage& usage) {
  DCHECK(thread_checker_.CalledOnValidThread());
  usage_ = usage;
}

}  // namespace remoting

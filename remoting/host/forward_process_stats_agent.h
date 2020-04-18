// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FORWARD_PROCESS_STATS_AGENT_H_
#define REMOTING_HOST_FORWARD_PROCESS_STATS_AGENT_H_

#include "base/threading/thread_checker.h"
#include "remoting/host/process_stats_agent.h"
#include "remoting/proto/process_stats.pb.h"

namespace remoting {

// ProcessStatsAgent implementation that returns stats passed to
// OnProcessStats(). Used to collect stats from processes other than the current
// one. All public functions, not including constructor and destructor, of the
// object of ForwardProcessStatsAgent are expected to be called in a same
// thread.
class ForwardProcessStatsAgent final : public ProcessStatsAgent {
 public:
  ForwardProcessStatsAgent();
  ~ForwardProcessStatsAgent() override;

  void OnProcessStats(const protocol::ProcessResourceUsage& usage);

  // ProcessStatsAgent implementation.
  protocol::ProcessResourceUsage GetResourceUsage() override;

 private:
  base::ThreadChecker thread_checker_;
  protocol::ProcessResourceUsage usage_;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FORWARD_PROCESS_STATS_AGENT_H_

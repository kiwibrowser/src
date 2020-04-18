// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_CURRENT_PROCESS_STATS_AGENT_H_
#define REMOTING_HOST_CURRENT_PROCESS_STATS_AGENT_H_

#include <memory>
#include <string>

#include "remoting/host/process_stats_agent.h"
#include "remoting/proto/process_stats.pb.h"

namespace base {
class ProcessMetrics;
}  // namespace base

namespace remoting {

// A component to report statistic data of the current process.
class CurrentProcessStatsAgent final : public ProcessStatsAgent {
 public:
  explicit CurrentProcessStatsAgent(const std::string& process_name);
  ~CurrentProcessStatsAgent() override;

  // ProcessStatsAgent implementation.
  protocol::ProcessResourceUsage GetResourceUsage() override;

 private:
  const std::string process_name_;
  const std::unique_ptr<base::ProcessMetrics> metrics_;
};

}  // namespace remoting

#endif  // REMOTING_HOST_CURRENT_PROCESS_STATS_AGENT_H_

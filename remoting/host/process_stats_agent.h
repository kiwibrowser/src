// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_PROCESS_STATS_AGENT_H_
#define REMOTING_HOST_PROCESS_STATS_AGENT_H_

#include "remoting/proto/process_stats.pb.h"

namespace remoting {

// An interface to report the process statistic data.
class ProcessStatsAgent {
 public:
  ProcessStatsAgent() = default;
  virtual ~ProcessStatsAgent() = default;

  // This function is expected to be executed in an IO-disallowed thread: if the
  // implementation requires IO access or may cost a significant amount of time,
  // using a background thread is preferred.
  virtual protocol::ProcessResourceUsage GetResourceUsage() = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_PROCESS_STATS_AGENT_H_

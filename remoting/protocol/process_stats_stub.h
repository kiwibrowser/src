// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_PROCESS_STATS_STUB_H_
#define REMOTING_PROTOCOL_PROCESS_STATS_STUB_H_

#include "remoting/proto/process_stats.pb.h"

namespace remoting {
namespace protocol {

// An interface to receive process statistic data.
class ProcessStatsStub {
 public:
  ProcessStatsStub() = default;
  virtual ~ProcessStatsStub() = default;

  virtual void OnProcessStats(
      const protocol::AggregatedProcessResourceUsage& usage) = 0;
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_PROCESS_STATS_STUB_H_

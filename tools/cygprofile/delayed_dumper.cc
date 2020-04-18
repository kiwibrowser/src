// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <time.h>
#include <unistd.h>

#include <thread>

#include "base/android/library_loader/anchor_functions.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "tools/cygprofile/lightweight_cygprofile.h"

#if !defined(ARCH_CPU_ARMEL)
#error Only supported on ARM.
#endif  // !defined(ARCH_CPU_ARMEL)

namespace cygprofile {
namespace {

// Disables the recording of addresses after |kDelayInSeconds| and dumps the
// result to a file.
class DelayedDumper {
 public:
  DelayedDumper() {
    // Not using base::TimeTicks() to not call too many base:: symbol that would
    // pollute the reached symbols dumps.
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts))
      PLOG(FATAL) << "clock_gettime.";
    uint64_t start_ns_since_epoch =
        static_cast<uint64_t>(ts.tv_sec) * 1000 * 1000 * 1000 + ts.tv_nsec;
    int pid = getpid();

    std::thread([pid, start_ns_since_epoch]() {
      sleep(kInitialDelayInSeconds);
      while (!SwitchToNextPhaseOrDump(pid, start_ns_since_epoch))
        sleep(kDelayInSeconds);
    })
        .detach();
  }

  static constexpr int kDelayInSeconds = 30;
  static constexpr int kInitialDelayInSeconds =
      kPhases == 1 ? kDelayInSeconds : 5;
};

// Static initializer on purpose. Will disable instrumentation after
// |kDelayInSeconds|.
DelayedDumper g_dump_later;

}  // namespace
}  // namespace cygprofile

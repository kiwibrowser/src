// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/seccomp_support_detector.h"

#include <stdio.h>
#include <sys/utsname.h>

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "sandbox/sandbox_buildflags.h"

#if BUILDFLAG(USE_SECCOMP_BPF)
#include "sandbox/linux/seccomp-bpf/sandbox_bpf.h"
#endif

namespace {

enum AndroidSeccompStatus {
  // DETECTION_FAILED was formerly used when probing for seccomp was done
  // out-of-process. There does not appear to be a gain in doing so, as
  // explained in the comment in DetectSeccomp(). This enum remains for
  // historical reasons.
  DETECTION_FAILED_OBSOLETE,  // The process crashed during detection.

  NOT_SUPPORTED,     // Kernel has no seccomp support.
  SUPPORTED,         // Kernel has seccomp support.
  LAST_STATUS
};

// Reports the kernel version obtained from uname.
void ReportKernelVersion() {
  // This method will report the kernel major and minor versions by
  // taking the lower 16 bits of each version number and combining
  // the two into a 32-bit number.

  utsname uts;
  if (uname(&uts) == 0) {
    int major, minor;
    if (sscanf(uts.release, "%d.%d", &major, &minor) == 2) {
      int version = ((major & 0xFFFF) << 16) | (minor & 0xFFFF);
      base::UmaHistogramSparse("Android.KernelVersion", version);
    }
  }
}

// Reports whether the system supports PR_SET_SECCOMP.
void ReportSeccompStatus() {
#if BUILDFLAG(USE_SECCOMP_BPF)
  bool prctl_supported = sandbox::SandboxBPF::SupportsSeccompSandbox(
      sandbox::SandboxBPF::SeccompLevel::SINGLE_THREADED);
#else
  bool prctl_supported = false;
#endif

  UMA_HISTOGRAM_ENUMERATION("Android.SeccompStatus.Prctl",
                            prctl_supported ? SUPPORTED : NOT_SUPPORTED,
                            LAST_STATUS);

  // Probing for the seccomp syscall can provoke kernel panics in certain LGE
  // devices. For now, this data will not be collected. In the future, this
  // should detect SeccompLevel::MULTI_THREADED. http://crbug.com/478478
}

}  // namespace

void ReportSeccompSupport() {
  ReportKernelVersion();
  ReportSeccompStatus();
}

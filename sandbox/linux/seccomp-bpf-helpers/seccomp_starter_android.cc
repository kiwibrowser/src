// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/seccomp-bpf-helpers/seccomp_starter_android.h"

#include <signal.h>
#include <string.h>

#include "base/logging.h"

#if BUILDFLAG(USE_SECCOMP_BPF)
#include "base/android/build_info.h"
#include "sandbox/linux/seccomp-bpf/sandbox_bpf.h"
#endif

namespace sandbox {

SeccompStarterAndroid::SeccompStarterAndroid(int build_sdk, const char* device)
    : sdk_int_(build_sdk), device_(device) {}

SeccompStarterAndroid::~SeccompStarterAndroid() = default;

bool SeccompStarterAndroid::StartSandbox() {
#if BUILDFLAG(USE_SECCOMP_BPF)
  DCHECK(policy_);

  if (!IsSupportedBySDK())
    return false;

  // Do run-time detection to ensure that support is present.
  if (!SandboxBPF::SupportsSeccompSandbox(
          SandboxBPF::SeccompLevel::MULTI_THREADED)) {
    status_ = SeccompSandboxStatus::DETECTION_FAILED;
    LOG(WARNING) << "Seccomp support should be present, but detection "
                 << "failed. Continuing without Seccomp-BPF.";
    return false;
  }

  sig_t old_handler = signal(SIGSYS, SIG_DFL);
  if (old_handler != SIG_DFL) {
    // On Android O and later, the zygote applies a seccomp filter to all
    // apps. It has its own SIGSYS handler that must be un-hooked so that
    // the Chromium one can be used instead. If pre-O devices have a SIGSYS
    // handler, then warn about that.
    DLOG_IF(WARNING, sdk_int_ < base::android::SDK_VERSION_OREO)
        << "Un-hooking existing SIGSYS handler before starting "
        << "Seccomp sandbox";
  }

  SandboxBPF sandbox(std::move(policy_));
  CHECK(sandbox.StartSandbox(SandboxBPF::SeccompLevel::MULTI_THREADED));
  status_ = SeccompSandboxStatus::ENGAGED;
  return true;
#else
  return false;
#endif
}

bool SeccompStarterAndroid::IsSupportedBySDK() const {
#if BUILDFLAG(USE_SECCOMP_BPF)
  if (sdk_int_ < base::android::SDK_VERSION_LOLLIPOP_MR1) {
    // Seccomp was never available pre-Lollipop.
    return false;
  }
  if (sdk_int_ > base::android::SDK_VERSION_LOLLIPOP_MR1) {
    // On Marshmallow and higher, Seccomp is required by CTS.
    return true;
  }
  // On Lollipop-MR1, only select Nexus devices have Seccomp available.
  const char* const kDevices[] = {
      "deb",   "flo",   "hammerhead", "mako",
      "manta", "shamu", "sprout",     "volantis",
  };
  for (auto* device : kDevices) {
    if (strcmp(device, device_) == 0)
      return true;
  }
#endif
  return false;
}

}  // namespace sandbox

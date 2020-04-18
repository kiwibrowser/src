// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/sandbox/linux/bpf_cros_amd_gpu_policy_linux.h"

#include <fcntl.h>
#include <sys/socket.h>

// Some arch's (arm64 for instance) unistd.h don't pull in symbols used here
// unless these are defined.
#define __ARCH_WANT_SYSCALL_NO_AT
#define __ARCH_WANT_SYSCALL_DEPRECATED
#include <unistd.h>

#include "base/logging.h"
#include "sandbox/linux/bpf_dsl/bpf_dsl.h"
#include "sandbox/linux/system_headers/linux_syscalls.h"

using sandbox::bpf_dsl::Allow;
using sandbox::bpf_dsl::Arg;
using sandbox::bpf_dsl::Error;
using sandbox::bpf_dsl::If;
using sandbox::bpf_dsl::ResultExpr;

namespace service_manager {

CrosAmdGpuProcessPolicy::CrosAmdGpuProcessPolicy() {}

CrosAmdGpuProcessPolicy::~CrosAmdGpuProcessPolicy() {}

ResultExpr CrosAmdGpuProcessPolicy::EvaluateSyscall(int sysno) const {
  switch (sysno) {
    case __NR_fstatfs:
    case __NR_sched_setscheduler:
    case __NR_sysinfo:
    case __NR_uname:
#if !defined(__aarch64__)
    case __NR_getdents:
    case __NR_readlink:
    case __NR_stat:
#endif
      return Allow();
#if defined(__x86_64__)
    // Allow only AF_UNIX for |domain|.
    case __NR_socket:
    case __NR_socketpair: {
      const Arg<int> domain(0);
      return If(domain == AF_UNIX, Allow()).Else(Error(EPERM));
    }
#endif
    default:
      // Default to the generic GPU policy.
      return GpuProcessPolicy::EvaluateSyscall(sysno);
  }
}

}  // namespace service_manager

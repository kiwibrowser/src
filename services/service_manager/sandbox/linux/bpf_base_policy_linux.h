// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_BPF_BASE_POLICY_LINUX_H_
#define SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_BPF_BASE_POLICY_LINUX_H_

#include <memory>

#include "base/macros.h"
#include "sandbox/linux/bpf_dsl/bpf_dsl_forward.h"
#include "sandbox/linux/bpf_dsl/policy.h"
#include "sandbox/linux/seccomp-bpf-helpers/baseline_policy.h"
#include "services/service_manager/sandbox/export.h"

namespace service_manager {

// The "baseline" BPF policy. Any other seccomp-bpf policy should inherit
// from it.
// It implements the main Policy interface. Due to its nature
// as a "kernel attack surface reduction" layer, it's implementation-defined.
class SERVICE_MANAGER_SANDBOX_EXPORT BPFBasePolicy
    : public sandbox::bpf_dsl::Policy {
 public:
  BPFBasePolicy();
  ~BPFBasePolicy() override;

  // sandbox::bpf_dsl::Policy:
  sandbox::bpf_dsl::ResultExpr EvaluateSyscall(
      int system_call_number) const override;
  sandbox::bpf_dsl::ResultExpr InvalidSyscall() const override;

  // Get the errno(3) to return for filesystem errors.
  static int GetFSDeniedErrno();

  pid_t GetPolicyPid() const { return baseline_policy_->policy_pid(); }

 private:
  // Compose the BaselinePolicy from sandbox/.
  std::unique_ptr<sandbox::BaselinePolicy> baseline_policy_;
  DISALLOW_COPY_AND_ASSIGN(BPFBasePolicy);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_BPF_BASE_POLICY_LINUX_H_

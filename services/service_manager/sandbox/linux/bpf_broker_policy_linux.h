// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_BPF_BROKER_POLICY_LINUX_H_
#define SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_BPF_BROKER_POLICY_LINUX_H_

#include "sandbox/linux/bpf_dsl/bpf_dsl.h"
#include "sandbox/linux/syscall_broker/broker_command.h"
#include "services/service_manager/sandbox/export.h"
#include "services/service_manager/sandbox/linux/bpf_base_policy_linux.h"

namespace service_manager {

// A broker policy is one for a privileged syscall broker that allows
// access, open, openat, and (in the non-Chrome OS case) unlink.
class SERVICE_MANAGER_SANDBOX_EXPORT BrokerProcessPolicy
    : public BPFBasePolicy {
 public:
  explicit BrokerProcessPolicy(
      const sandbox::syscall_broker::BrokerCommandSet& allowed_command_set);
  ~BrokerProcessPolicy() override;

  sandbox::bpf_dsl::ResultExpr EvaluateSyscall(
      int system_call_number) const override;

 private:
  const sandbox::syscall_broker::BrokerCommandSet allowed_command_set_;

  DISALLOW_COPY_AND_ASSIGN(BrokerProcessPolicy);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_BPF_BROKER_POLICY_LINUX_H_

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/sandbox/linux/bpf_base_policy_linux.h"

#include <errno.h>

#include "base/logging.h"
#include "sandbox/linux/bpf_dsl/bpf_dsl.h"
#include "sandbox/linux/seccomp-bpf-helpers/baseline_policy.h"

using sandbox::bpf_dsl::ResultExpr;

namespace service_manager {

namespace {

// The errno used for denied file system access system calls, such as open(2).
static const int kFSDeniedErrno = EPERM;

}  // namespace.

BPFBasePolicy::BPFBasePolicy()
    : baseline_policy_(new sandbox::BaselinePolicy(kFSDeniedErrno)) {}
BPFBasePolicy::~BPFBasePolicy() {}

ResultExpr BPFBasePolicy::EvaluateSyscall(int system_call_number) const {
  DCHECK(baseline_policy_);
  return baseline_policy_->EvaluateSyscall(system_call_number);
}

ResultExpr BPFBasePolicy::InvalidSyscall() const {
  DCHECK(baseline_policy_);
  return baseline_policy_->InvalidSyscall();
}

int BPFBasePolicy::GetFSDeniedErrno() {
  return kFSDeniedErrno;
}

}  // namespace service_manager.

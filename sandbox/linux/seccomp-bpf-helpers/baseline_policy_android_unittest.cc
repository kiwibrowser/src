// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/seccomp-bpf-helpers/baseline_policy_android.h"

#include <sys/resource.h>

#include "sandbox/linux/seccomp-bpf/bpf_tests.h"

namespace sandbox {
namespace {

BPF_TEST_C(BaselinePolicyAndroid, Getrusage, BaselinePolicyAndroid) {
  struct rusage usage{};

  errno = 0;
  BPF_ASSERT_EQ(0, getrusage(RUSAGE_SELF, &usage));

  errno = 0;
  BPF_ASSERT_EQ(0, getrusage(RUSAGE_THREAD, &usage));
}

}  // namespace
}  // namespace sandbox

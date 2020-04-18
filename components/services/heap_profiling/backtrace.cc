// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/backtrace.h"

#include <string.h>

#include <algorithm>

#include "base/hash.h"
#include "components/services/heap_profiling/backtrace_storage.h"

namespace heap_profiling {

namespace {

// TODO(ajwong) replace with a fingerprint capable hash.
size_t ComputeHash(const std::vector<Address>& addrs) {
  if (addrs.empty())
    return 0;
  // Assume Address is a POD containing only the address with no padding.
  return base::Hash(addrs.data(), addrs.size() * sizeof(Address));
}

}  // namespace

Backtrace::Backtrace(std::vector<Address>&& a)
    : addrs_(std::move(a)), fingerprint_(ComputeHash(addrs_)) {}

Backtrace::Backtrace(Backtrace&& other) noexcept = default;

Backtrace::~Backtrace() {}

Backtrace& Backtrace::operator=(Backtrace&& other) = default;

bool Backtrace::operator==(const Backtrace& other) const {
  if (addrs_.size() != other.addrs_.size())
    return false;
  return memcmp(addrs_.data(), other.addrs_.data(),
                addrs_.size() * sizeof(Address)) == 0;
}

bool Backtrace::operator!=(const Backtrace& other) const {
  return !operator==(other);
}

}  // namespace heap_profiling

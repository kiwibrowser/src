// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_BACKTRACE_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_BACKTRACE_H_

#include <functional>
#include <vector>

#include "base/macros.h"
#include "components/services/heap_profiling/address.h"

namespace heap_profiling {

// Holds a move-only stack backtrace and a precomputed hash. This backtrace
// uses addresses in the instrumented process. This is in contrast to
// base::StackTrace which is for getting and working with stack traces in the
// current process.
//
// This is immutable since we assume it can be read from multiple threads
// without locking.
//
// This class has a ref_count member which is used by the allocation tracker
// to track references to the stack. The reference counting is managed
// externally. Tracking live objects with a global atom list in a threadsafe
// manner is much more difficult if this class derives from RefCount.
class Backtrace {
 public:
  // Move-only class. Backtraces should be managed by BacktraceStorage and
  // we shouldn't be copying vectors around.
  explicit Backtrace(std::vector<Address>&& a);
  Backtrace(Backtrace&& other) noexcept;
  ~Backtrace();

  Backtrace& operator=(Backtrace&& other);

  bool operator==(const Backtrace& other) const;
  bool operator!=(const Backtrace& other) const;

  const std::vector<Address>& addrs() const { return addrs_; }

  size_t fingerprint() const { return fingerprint_; }

 private:
  friend class BacktraceStorage;  // Only BacktraceStorage can do ref counting.

  // The reference counting is not threadsafe. it's assumed the
  // BacktraceStorage is the only class accessing this, and it's done inside a
  // lock.
  void AddRef() const { ref_count_++; }
  bool Release() const {  // Returns whether the result is non-zero.
    return !!(--ref_count_);
  }

  std::vector<Address> addrs_;
  size_t fingerprint_;
  mutable int ref_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(Backtrace);
};

}  // namespace heap_profiling

namespace std {

template <>
struct hash<heap_profiling::Backtrace> {
  using argument_type = heap_profiling::Backtrace;
  using result_type = size_t;
  result_type operator()(const argument_type& s) const {
    return s.fingerprint();
  }
};

}  // namespace std

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_BACKTRACE_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_ADDRESS_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_ADDRESS_H_

#include <stdint.h>

#include <functional>
#include <iosfwd>

#include "base/hash.h"

namespace heap_profiling {

// Wrapper around an address in the instrumented process. This wrapper should
// be a zero-overhead abstraction around a 64-bit integer (so pass by value)
// that prevents getting confused between addresses in the local process and
// ones in the instrumented process.
struct Address {
  Address() : value(0) {}
  explicit Address(uint64_t v) : value(v) {}

  uint64_t value;

  bool operator<(Address other) const { return value < other.value; }
  bool operator<=(Address other) const { return value <= other.value; }
  bool operator>(Address other) const { return value > other.value; }
  bool operator>=(Address other) const { return value >= other.value; }

  bool operator==(Address other) const { return value == other.value; }
  bool operator!=(Address other) const { return value != other.value; }

  Address operator+(int64_t delta) const { return Address(value + delta); }
  Address operator+=(int64_t delta) {
    value += delta;
    return *this;
  }

  Address operator-(int64_t delta) const { return Address(value - delta); }
  Address operator-=(int64_t delta) {
    value -= delta;
    return *this;
  }

  int64_t operator-(Address a) const { return value - a.value; }
};

}  // namespace heap_profiling

namespace std {

template <>
struct hash<heap_profiling::Address> {
  typedef heap_profiling::Address argument_type;
  typedef uint32_t result_type;
  result_type operator()(argument_type a) const {
    return base::Hash(&a.value, sizeof(int64_t));
  }
};

}  // namespace std

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_ADDRESS_H_

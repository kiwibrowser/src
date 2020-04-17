// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_FRAME_ID_H_
#define STREAMING_CAST_FRAME_ID_H_

#include <stdint.h>

#include <sstream>

#include "streaming/cast/expanded_value_base.h"

namespace openscreen {
namespace cast_streaming {

// Forward declaration (see below).
class FrameId;

// Convenience operator overloads for logging.
std::ostream& operator<<(std::ostream& out, const FrameId rhs);

// Unique identifier for a frame in a RTP media stream.  FrameIds are truncated
// to 8-bit values in RTP and RTCP headers, and then expanded back by the other
// endpoint when parsing the headers.
//
// Usage example:
//
//   // Distance/offset math.
//   FrameId first = FrameId::first();
//   FrameId second = first + 1;
//   FrameId third = second + 1;
//   int64_t offset = third - first;
//   FrameId fourth = second + offset;
//
//   // Logging convenience.
//   OSP_DLOG_INFO << "The current frame is " << fourth;
class FrameId : public ExpandedValueBase<int64_t, FrameId> {
 public:
  // The "null" FrameId constructor.  Represents a FrameId field that has not
  // been set and/or a "not applicable" indicator.
  constexpr FrameId() : FrameId(std::numeric_limits<int64_t>::min()) {}

  // Allow copy construction and assignment.
  constexpr FrameId(const FrameId&) = default;
  constexpr FrameId& operator=(const FrameId&) = default;

  // Returns true if this is the special value representing null.
  constexpr bool is_null() const { return *this == FrameId(); }

  // Distance operator.
  int64_t operator-(FrameId rhs) const {
    OSP_DCHECK(!is_null());
    OSP_DCHECK(!rhs.is_null());
    return value_ - rhs.value_;
  }

  // Operators to compute advancement by incremental amounts.
  FrameId operator+(int64_t rhs) const {
    OSP_DCHECK(!is_null());
    return FrameId(value_ + rhs);
  }
  FrameId operator-(int64_t rhs) const {
    OSP_DCHECK(!is_null());
    return FrameId(value_ - rhs);
  }
  FrameId& operator+=(int64_t rhs) {
    OSP_DCHECK(!is_null());
    return (*this = (*this + rhs));
  }
  FrameId& operator-=(int64_t rhs) {
    OSP_DCHECK(!is_null());
    return (*this = (*this - rhs));
  }
  FrameId& operator++() {
    OSP_DCHECK(!is_null());
    ++value_;
    return *this;
  }
  FrameId& operator--() {
    OSP_DCHECK(!is_null());
    --value_;
    return *this;
  }
  FrameId operator++(int) {
    OSP_DCHECK(!is_null());
    return FrameId(value_++);
  }
  FrameId operator--(int) {
    OSP_DCHECK(!is_null());
    return FrameId(value_--);
  }

  // The identifier for the first frame in a stream.
  static constexpr FrameId first() { return FrameId(0); }

 private:
  friend class ExpandedValueBase<int64_t, FrameId>;
  friend std::ostream& operator<<(std::ostream& out, const FrameId rhs);

  constexpr explicit FrameId(int64_t value) : ExpandedValueBase(value) {}

  // Accessor used by ostream output function.
  constexpr int64_t value() const { return value_; }
};

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_FRAME_ID_H_

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_COMMON_FRAME_ID_H_
#define MEDIA_CAST_COMMON_FRAME_ID_H_

#include <stdint.h>

#include <sstream>

#include "media/cast/common/expanded_value_base.h"

namespace media {
namespace cast {

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
//   DLOG(INFO) << "The current frame is " << fourth;
class FrameId : public ExpandedValueBase<int64_t, FrameId> {
 public:
  // The "null" FrameId constructor.  Represents a FrameId field that has not
  // been set and/or a "not applicable" indicator.
  FrameId() : FrameId(std::numeric_limits<int64_t>::min()) {}

  // Allow copy construction and assignment.
  FrameId(const FrameId&) = default;
  FrameId& operator=(const FrameId&) = default;

  // Returns true if this is the special value representing null.
  bool is_null() const { return *this == FrameId(); }

  // Distance operator.
  int64_t operator-(FrameId rhs) const {
    DCHECK(!is_null());
    DCHECK(!rhs.is_null());
    return value_ - rhs.value_;
  }

  // Operators to compute advancement by incremental amounts.
  FrameId operator+(int64_t rhs) const {
    DCHECK(!is_null());
    return FrameId(value_ + rhs);
  }
  FrameId operator-(int64_t rhs) const {
    DCHECK(!is_null());
    return FrameId(value_ - rhs);
  }
  FrameId& operator+=(int64_t rhs) {
    DCHECK(!is_null());
    return (*this = (*this + rhs));
  }
  FrameId& operator-=(int64_t rhs) {
    DCHECK(!is_null());
    return (*this = (*this - rhs));
  }
  FrameId& operator++() {
    DCHECK(!is_null());
    ++value_;
    return *this;
  }
  FrameId& operator--() {
    DCHECK(!is_null());
    --value_;
    return *this;
  }
  FrameId operator++(int) {
    DCHECK(!is_null());
    return FrameId(value_++);
  }
  FrameId operator--(int) {
    DCHECK(!is_null());
    return FrameId(value_--);
  }

  // The identifier for the first frame in a stream.
  static FrameId first() { return FrameId(0); }

 private:
  friend class ExpandedValueBase<int64_t, FrameId>;
  friend std::ostream& operator<<(std::ostream& out, const FrameId rhs);

  explicit FrameId(int64_t value) : ExpandedValueBase(value) {}

  // Accessor used by ostream output function.
  int64_t value() const { return value_; }
};

}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_COMMON_FRAME_ID_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBRTC_OVERRIDES_RTC_BASE_EVENT_H_
#define THIRD_PARTY_WEBRTC_OVERRIDES_RTC_BASE_EVENT_H_

#include "base/macros.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_restrictions.h"

namespace rtc {

// Overrides WebRTC's internal event implementation to use Chromium's.
class Event {
 public:
  static const int kForever = -1;

  Event(bool manual_reset, bool initially_signaled);
  ~Event();

  void Set();
  void Reset();

  // Wait for the event to become signaled, for the specified number of
  // |milliseconds|.  To wait indefinetly, pass kForever.
  bool Wait(int milliseconds);

 private:
  base::WaitableEvent event_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(Event);
};

// Pull ScopedAllowBaseSyncPrimitives into the rtc namespace.
// Managing what types in WebRTC are allowed to use
// ScopedAllowBaseSyncPrimitives, is done via thread_restrictions.h.
using base::ScopedAllowBaseSyncPrimitives;

}  // namespace rtc

#endif  // THIRD_PARTY_WEBRTC_OVERRIDES_RTC_BASE_EVENT_H_

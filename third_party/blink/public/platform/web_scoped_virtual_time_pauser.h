// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SCOPED_VIRTUAL_TIME_PAUSER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SCOPED_VIRTUAL_TIME_PAUSER_H_

#include "base/time/time.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {
namespace scheduler {
class MainThreadSchedulerImpl;
}  // namespace scheduler

// A move only RAII style helper which makes it easier for subsystems to pause
// virtual time while performing an asynchronous operation.
class BLINK_PLATFORM_EXPORT WebScopedVirtualTimePauser {
 public:
  enum class VirtualTaskDuration {
    kInstant,    // Virtual time will not be advanced when it's unpaused.
    kNonInstant  // Virtual time may be advanced when it's unpaused.
  };

  // Note simply creating a WebScopedVirtualTimePauser doesn't cause VirtualTime
  // to pause, instead you need to call PauseVirtualTime.
  WebScopedVirtualTimePauser(scheduler::MainThreadSchedulerImpl*,
                             VirtualTaskDuration,
                             const WebString& debug_name);

  WebScopedVirtualTimePauser();
  ~WebScopedVirtualTimePauser();

  WebScopedVirtualTimePauser(WebScopedVirtualTimePauser&& other);
  WebScopedVirtualTimePauser& operator=(WebScopedVirtualTimePauser&& other);

  WebScopedVirtualTimePauser(const WebScopedVirtualTimePauser&) = delete;
  WebScopedVirtualTimePauser& operator=(const WebScopedVirtualTimePauser&) =
      delete;

  // Virtual time will be paused if any WebScopedVirtualTimePauser votes to
  // pause it, and only unpaused only if all WebScopedVirtualTimePauser are
  // either destroyed or vote to unpause.
  void PauseVirtualTime();
  void UnpauseVirtualTime();

 private:
  void DecrementVirtualTimePauseCount();

  base::TimeTicks virtual_time_when_paused_;
  bool paused_ = false;
  VirtualTaskDuration duration_ = VirtualTaskDuration::kInstant;
  scheduler::MainThreadSchedulerImpl* scheduler_;  // NOT OWNED
  WebString debug_name_;
  int trace_id_;

  static int next_trace_id_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SCOPED_VIRTUAL_TIME_PAUSER_H_

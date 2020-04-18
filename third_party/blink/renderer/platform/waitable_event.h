/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WAITABLE_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WAITABLE_EVENT_H_

#include <memory>
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace base {
class WaitableEvent;
};  // namespace base

namespace blink {

// TODO(crbug.com/796799): Deprecate blink::WaitableEvent and use
// base::WaitableEvent instead.
//
// Provides a thread synchronization that can be used to allow one thread to
// wait until another thread to finish some work.
class PLATFORM_EXPORT WaitableEvent {
 public:
  // If ResetPolicy::Manual is specified on creation, to set the event state
  // to non-signaled, a consumer must call reset().  Otherwise, the system
  // automatically resets the event state to non-signaled after a single
  // waiting thread has been released.
  enum class ResetPolicy { kAuto, kManual };

  // Specify the initial state on creation.
  enum class InitialState { kNonSignaled, kSignaled };

  explicit WaitableEvent(ResetPolicy = ResetPolicy::kAuto,
                         InitialState = InitialState::kNonSignaled);

  ~WaitableEvent();

  // Puts the event in the un-signaled state.
  void Reset();

  // Waits indefinitely for the event to be signaled.
  void Wait();

  // Puts the event in the signaled state. Causing any thread blocked on Wait
  // to be woken up. The event state is reset to non-signaled after
  // a waiting thread has been released.
  void Signal();

  // Waits on multiple events and returns the index of the object that
  // has been signaled. Any event objects given to this method must
  // not deleted while this wait is happening.
  static size_t WaitMultiple(const WTF::Vector<WaitableEvent*>& events);

 private:
  WaitableEvent(const WaitableEvent&) = delete;
  void operator=(const WaitableEvent&) = delete;

  std::unique_ptr<base::WaitableEvent> impl_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WAITABLE_EVENT_H_

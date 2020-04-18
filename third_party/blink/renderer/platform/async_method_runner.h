/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_ASYNC_METHOD_RUNNER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_ASYNC_METHOD_RUNNER_H_

#include "base/single_thread_task_runner.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

template <typename TargetClass>
class AsyncMethodRunner final
    : public GarbageCollectedFinalized<AsyncMethodRunner<TargetClass>> {
  WTF_MAKE_NONCOPYABLE(AsyncMethodRunner);

 public:
  typedef void (TargetClass::*TargetMethod)();

  static AsyncMethodRunner* Create(
      TargetClass* object,
      TargetMethod method,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
    return new AsyncMethodRunner(object, method, std::move(task_runner));
  }

  ~AsyncMethodRunner() = default;

  // Schedules to run the method asynchronously. Do nothing if it's already
  // scheduled. If it's suspended, remember to schedule to run the method when
  // Unpause() is called.
  void RunAsync() {
    if (paused_) {
      DCHECK(!timer_.IsActive());
      run_when_unpaused_ = true;
      return;
    }

    // FIXME: runAsync should take a TraceLocation and pass it to timer here.
    if (!timer_.IsActive())
      timer_.StartOneShot(TimeDelta(), FROM_HERE);
  }

  // If it's scheduled to run the method, cancel it and remember to schedule
  // it again when resume() is called. Mainly for implementing
  // PausableObject::Pause().
  void Pause() {
    if (paused_)
      return;
    paused_ = true;

    if (!timer_.IsActive())
      return;

    timer_.Stop();
    run_when_unpaused_ = true;
  }

  // Resumes pending method run.
  void Unpause() {
    if (!paused_)
      return;
    paused_ = false;

    if (!run_when_unpaused_)
      return;

    run_when_unpaused_ = false;
    // FIXME: resume should take a TraceLocation and pass it to timer here.
    timer_.StartOneShot(TimeDelta(), FROM_HERE);
  }

  void Stop() {
    if (paused_) {
      DCHECK(!timer_.IsActive());
      run_when_unpaused_ = false;
      paused_ = false;
      return;
    }

    DCHECK(!run_when_unpaused_);
    timer_.Stop();
  }

  bool IsActive() const { return timer_.IsActive(); }

  void Trace(blink::Visitor* visitor) { visitor->Trace(object_); }

 private:
  AsyncMethodRunner(TargetClass* object,
                    TargetMethod method,
                    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : timer_(std::move(task_runner),
               this,
               &AsyncMethodRunner<TargetClass>::Fired),
        object_(object),
        method_(method),
        paused_(false),
        run_when_unpaused_(false) {}

  void Fired(TimerBase*) { (object_->*method_)(); }

  TaskRunnerTimer<AsyncMethodRunner<TargetClass>> timer_;

  Member<TargetClass> object_;
  TargetMethod method_;

  bool paused_;
  bool run_when_unpaused_;
};

}  // namespace blink

#endif

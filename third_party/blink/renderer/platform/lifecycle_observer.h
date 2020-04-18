/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LIFECYCLE_OBSERVER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LIFECYCLE_OBSERVER_H_

#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

template <typename Context, typename Observer>
class LifecycleNotifier;

template <typename Context, typename Observer>
class LifecycleObserver : public GarbageCollectedMixin {
 public:
  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(lifecycle_context_);
  }

  Context* LifecycleContext() const { return lifecycle_context_; }

  void ClearContext() { SetContext(nullptr); }

 protected:
  explicit LifecycleObserver(Context* context) : lifecycle_context_(nullptr) {
    SetContext(context);
  }

  void SetContext(Context*);

 private:
  WeakMember<Context> lifecycle_context_;
};

template <typename Context, typename Observer>
inline void LifecycleObserver<Context, Observer>::SetContext(Context* context) {
  using Notifier = LifecycleNotifier<Context, Observer>;

  if (lifecycle_context_ == context)
    return;

  if (lifecycle_context_) {
    static_cast<Notifier*>(lifecycle_context_)
        ->RemoveObserver(static_cast<Observer*>(this));
  }

  lifecycle_context_ = context;

  if (lifecycle_context_) {
    static_cast<Notifier*>(lifecycle_context_)
        ->AddObserver(static_cast<Observer*>(this));
  }
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_LIFECYCLE_OBSERVER_H_

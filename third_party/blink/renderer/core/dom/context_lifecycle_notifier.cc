/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2013 Google Inc. All Rights Reserved.
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

#include "third_party/blink/renderer/core/dom/context_lifecycle_notifier.h"

#include "base/auto_reset.h"
#include "third_party/blink/renderer/core/dom/pausable_object.h"

namespace blink {

void ContextLifecycleNotifier::NotifyResumingPausableObjects() {
  base::AutoReset<IterationState> scope(&iteration_state_, kAllowingNone);
  for (ContextLifecycleObserver* observer : observers_) {
    if (observer->ObserverType() !=
        ContextLifecycleObserver::kPausableObjectType)
      continue;
    PausableObject* pausable_object = static_cast<PausableObject*>(observer);
#if DCHECK_IS_ON()
    DCHECK_EQ(pausable_object->GetExecutionContext(), Context());
    DCHECK(pausable_object->PauseIfNeededCalled());
#endif
    pausable_object->Unpause();
  }
}

void ContextLifecycleNotifier::NotifySuspendingPausableObjects() {
  base::AutoReset<IterationState> scope(&iteration_state_, kAllowingNone);
  for (ContextLifecycleObserver* observer : observers_) {
    if (observer->ObserverType() !=
        ContextLifecycleObserver::kPausableObjectType)
      continue;
    PausableObject* pausable_object = static_cast<PausableObject*>(observer);
#if DCHECK_IS_ON()
    DCHECK_EQ(pausable_object->GetExecutionContext(), Context());
    DCHECK(pausable_object->PauseIfNeededCalled());
#endif
    pausable_object->Pause();
  }
}

unsigned ContextLifecycleNotifier::PausableObjectCount() const {
  DCHECK(!IsIteratingOverObservers());
  unsigned pausable_objects = 0;
  for (ContextLifecycleObserver* observer : observers_) {
    if (observer->ObserverType() !=
        ContextLifecycleObserver::kPausableObjectType)
      continue;
    pausable_objects++;
  }
  return pausable_objects;
}

#if DCHECK_IS_ON()
bool ContextLifecycleNotifier::Contains(PausableObject* object) const {
  DCHECK(!IsIteratingOverObservers());
  for (ContextLifecycleObserver* observer : observers_) {
    if (observer->ObserverType() !=
        ContextLifecycleObserver::kPausableObjectType)
      continue;
    PausableObject* pausable_object = static_cast<PausableObject*>(observer);
    if (pausable_object == object)
      return true;
  }
  return false;
}
#endif

}  // namespace blink

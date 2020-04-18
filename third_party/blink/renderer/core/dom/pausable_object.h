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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_PAUSABLE_OBJECT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_PAUSABLE_OBJECT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

class CORE_EXPORT PausableObject : public ContextLifecycleObserver {
 public:
  explicit PausableObject(ExecutionContext*);

  // PauseIfNeeded() should be called exactly once after object construction
  // to synchronize the suspend state with that in ExecutionContext.
  void PauseIfNeeded();
#if DCHECK_IS_ON()
  bool PauseIfNeededCalled() const { return pause_if_needed_called_; }
#endif

  // These methods have an empty default implementation so that subclasses
  // which don't need special treatment can skip implementation.
  virtual void Pause();
  virtual void Unpause();

  void DidMoveToNewExecutionContext(ExecutionContext*);

 protected:
  virtual ~PausableObject();

 private:
#if DCHECK_IS_ON()
  bool pause_if_needed_called_;
#endif
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_PAUSABLE_OBJECT_H_

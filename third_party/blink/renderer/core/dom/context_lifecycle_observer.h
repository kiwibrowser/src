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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_CONTEXT_LIFECYCLE_OBSERVER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_CONTEXT_LIFECYCLE_OBSERVER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/lifecycle_observer.h"

namespace blink {

class LocalDOMWindow;
class LocalFrame;

// ContextClient and ContextLifecycleObserver are helpers to associate an
// object with an ExecutionContext.
//
// - getExecutionContext() returns null after the context is detached.
// - frame() is a syntax sugar for getExecutionContext()->frame(). It returns
//   null after the context is detached or the context is not a Document.
//
// Both can safely be used up until destruction; i.e., unsafe to
// call upon in a destructor.
class CORE_EXPORT ContextClient : public GarbageCollectedMixin {
 public:
  ExecutionContext* GetExecutionContext() const;
  LocalFrame* GetFrame() const;

  void Trace(blink::Visitor*) override;

 protected:
  explicit ContextClient(ExecutionContext*);
  explicit ContextClient(LocalFrame*);

 private:
  WeakMember<ExecutionContext> execution_context_;
};

// ContextLifecycleObserver provides an additional contextDestroyed() hook
// to execute cleanup code when a context is destroyed. Prefer the simpler
// ContextClient when possible.
class CORE_EXPORT ContextLifecycleObserver
    : public LifecycleObserver<ExecutionContext, ContextLifecycleObserver> {
 public:
  virtual void ContextDestroyed(ExecutionContext*) {}

  ExecutionContext* GetExecutionContext() const { return LifecycleContext(); }
  LocalFrame* GetFrame() const;

  enum Type {
    kGenericType,
    kPausableObjectType,
  };

  Type ObserverType() const { return observer_type_; }

 protected:
  explicit ContextLifecycleObserver(ExecutionContext* execution_context,
                                    Type type = kGenericType)
      : LifecycleObserver(execution_context), observer_type_(type) {}

 private:
  Type observer_type_;
};

// DOMWindowClient is a helper to associate an object with a LocalDOMWindow.
//
// - domWindow() returns null after the window is detached.
// - frame() is a syntax sugar for domWindow()->frame(). It returns
//   null after the window is detached.
//
// Both can safely be used up until destruction; i.e., unsafe to
// call upon in a destructor.
//
// If the object is a per-ExecutionContext thing, use ContextClient/
// ContextLifecycleObserver. If the object is a per-DOMWindow thing, use
// DOMWindowClient. Basically, DOMWindowClient is expected to be used (only)
// for objects directly held by LocalDOMWindow. Other objects should use
// ContextClient/ContextLifecycleObserver.
//
// There is a subtle difference between the timing when the context gets
// detached and the timing when the window gets detached. In common cases,
// these two happen at the same timing. The only exception is a case where
// a frame navigates from an initial empty document to another same-origin
// document. In this case, a Document is recreated but a DOMWindow is reused.
// Hence, in the navigated document ContextClient::getExecutionContext()
// returns null while DOMWindowClient::domWindow() keeps returning the window.
class CORE_EXPORT DOMWindowClient : public GarbageCollectedMixin {
 public:
  LocalDOMWindow* DomWindow() const;
  LocalFrame* GetFrame() const;

  void Trace(blink::Visitor*) override;

 protected:
  explicit DOMWindowClient(LocalDOMWindow*);
  explicit DOMWindowClient(LocalFrame*);

 private:
  WeakMember<LocalDOMWindow> dom_window_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_CONTEXT_LIFECYCLE_OBSERVER_H_

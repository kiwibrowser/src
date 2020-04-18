// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_ELEMENT_VISIBILITY_OBSERVER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_ELEMENT_VISIBILITY_OBSERVER_H_

#include <limits>

#include "base/callback.h"
#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/heap/member.h"

namespace blink {

class Element;

// ElementVisibilityObserver is a helper class to be used to track the
// visibility of an Element in the viewport. Creating an
// ElementVisibilityObserver is a no-op with regards to CPU cycle. The observing
// has be started by calling |start()| and can be stopped with |stop()|.
// When creating an instance, the caller will have to pass a callback taking
// a boolean as an argument. The boolean will be the new visibility state.
// The ElementVisibilityObserver is implemented on top of IntersectionObserver.
// It is a layer meant to simplify the usage for C++ Blink code checking for the
// visibility of an element.
class CORE_EXPORT ElementVisibilityObserver final
    : public GarbageCollectedFinalized<ElementVisibilityObserver> {
 public:
  using VisibilityCallback = base::RepeatingCallback<void(bool)>;

  ElementVisibilityObserver(Element*, VisibilityCallback);
  virtual ~ElementVisibilityObserver();

  // The |threshold| is the minimum fraction that needs to be visible.
  // See https://github.com/WICG/IntersectionObserver/issues/164 for why this
  // defaults to std::numeric_limits<float>::min() rather than zero.
  void Start(float threshold = std::numeric_limits<float>::min());
  void Stop();

  void DeliverObservationsForTesting();

  virtual void Trace(blink::Visitor*);

 private:
  class ElementVisibilityCallback;

  void OnVisibilityChanged(
      const HeapVector<Member<IntersectionObserverEntry>>&);

  Member<Element> element_;
  Member<IntersectionObserver> intersection_observer_;
  VisibilityCallback callback_;
  DISALLOW_COPY_AND_ASSIGN(ElementVisibilityObserver);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_ELEMENT_VISIBILITY_OBSERVER_H_

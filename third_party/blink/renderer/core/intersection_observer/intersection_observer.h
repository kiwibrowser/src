// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_H_

#include "base/callback.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observation.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer_entry.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/length.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class Document;
class Element;
class ExceptionState;
class IntersectionObserverDelegate;
class IntersectionObserverInit;
class ScriptState;
class V8IntersectionObserverCallback;

class CORE_EXPORT IntersectionObserver final
    : public ScriptWrappable,
      public ActiveScriptWrappable<IntersectionObserver>,
      public ContextClient {
  USING_GARBAGE_COLLECTED_MIXIN(IntersectionObserver);
  DEFINE_WRAPPERTYPEINFO();

 public:
  using EventCallback = base::RepeatingCallback<void(
      const HeapVector<Member<IntersectionObserverEntry>>&)>;

  static IntersectionObserver* Create(const IntersectionObserverInit&,
                                      IntersectionObserverDelegate&,
                                      ExceptionState&);
  static IntersectionObserver* Create(ScriptState*,
                                      V8IntersectionObserverCallback*,
                                      const IntersectionObserverInit&,
                                      ExceptionState&);
  static IntersectionObserver* Create(const Vector<Length>& root_margin,
                                      const Vector<float>& thresholds,
                                      Document*,
                                      EventCallback,
                                      bool track_visbility = false,
                                      ExceptionState& = ASSERT_NO_EXCEPTION);
  static void ResumeSuspendedObservers();

  // API methods.
  void observe(Element*, ExceptionState& = ASSERT_NO_EXCEPTION);
  void unobserve(Element*, ExceptionState& = ASSERT_NO_EXCEPTION);
  void disconnect(ExceptionState& = ASSERT_NO_EXCEPTION);
  HeapVector<Member<IntersectionObserverEntry>> takeRecords(ExceptionState&);

  // API attributes.
  Element* root() const { return root_.Get(); }
  String rootMargin() const;
  const Vector<float>& thresholds() const { return thresholds_; }
  bool trackVisibility() const { return track_visibility_; }

  // An observer can either track intersections with an explicit root Element,
  // or with the the top-level frame's viewport (the "implicit root").  When
  // tracking the implicit root, m_root will be null, but because m_root is a
  // weak pointer, we cannot surmise that this observer tracks the implicit
  // root just because m_root is null.  Hence m_rootIsImplicit.
  bool RootIsImplicit() const { return root_is_implicit_; }

  // This is the document which is responsible for running
  // computeIntersectionObservations at frame generation time.
  // This can return nullptr when no tracking document is available.
  Document* TrackingDocument() const;

  const Length& TopMargin() const { return top_margin_; }
  const Length& RightMargin() const { return right_margin_; }
  const Length& BottomMargin() const { return bottom_margin_; }
  const Length& LeftMargin() const { return left_margin_; }
  void ComputeIntersectionObservations();
  void EnqueueIntersectionObserverEntry(IntersectionObserverEntry&);
  unsigned FirstThresholdGreaterThan(float ratio) const;
  void Deliver();
  bool HasEntries() const { return entries_.size(); }
  const HeapLinkedHashSet<WeakMember<IntersectionObservation>>& Observations()
      const {
    return observations_;
  }

  // ScriptWrappable override:
  bool HasPendingActivity() const override;

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;

 private:
  explicit IntersectionObserver(IntersectionObserverDelegate&,
                                Element*,
                                const Vector<Length>& root_margin,
                                const Vector<float>& thresholds,
                                bool track_visibility);
  void ClearWeakMembers(Visitor*);

  // Returns false if this observer has an explicit root element which has been
  // deleted; true otherwise.
  bool RootIsValid() const;

  const TraceWrapperMember<IntersectionObserverDelegate> delegate_;
  WeakMember<Element> root_;
  HeapLinkedHashSet<WeakMember<IntersectionObservation>> observations_;
  HeapVector<Member<IntersectionObserverEntry>> entries_;
  Vector<float> thresholds_;
  Length top_margin_;
  Length right_margin_;
  Length bottom_margin_;
  Length left_margin_;
  unsigned root_is_implicit_ : 1;
  unsigned track_visibility_ : 1;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_INTERSECTION_OBSERVER_INTERSECTION_OBSERVER_H_

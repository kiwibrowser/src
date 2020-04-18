// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PRESENTATION_PRESENTATION_AVAILABILITY_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PRESENTATION_PRESENTATION_AVAILABILITY_H_

#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/dom/pausable_object.h"
#include "third_party/blink/renderer/core/page/page_visibility_observer.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/presentation/presentation_availability_observer.h"
#include "third_party/blink/renderer/modules/presentation/presentation_promise_property.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class ExecutionContext;

// Expose whether there is a presentation display available for |url|. The
// object will be initialized with a default value passed via ::take() and will
// then subscribe to receive callbacks if the status for |url| were to
// change. The object will only listen to changes when required.
class MODULES_EXPORT PresentationAvailability final
    : public EventTargetWithInlineData,
      public ActiveScriptWrappable<PresentationAvailability>,
      public PausableObject,
      public PageVisibilityObserver,
      public PresentationAvailabilityObserver {
  USING_GARBAGE_COLLECTED_MIXIN(PresentationAvailability);
  DEFINE_WRAPPERTYPEINFO();

 public:
  static PresentationAvailability* Take(PresentationAvailabilityProperty*,
                                        const WTF::Vector<KURL>&,
                                        bool);
  ~PresentationAvailability() override;

  // EventTarget implementation.
  const AtomicString& InterfaceName() const override;
  ExecutionContext* GetExecutionContext() const override;

  // PresentationAvailabilityObserver implementation.
  void AvailabilityChanged(blink::mojom::ScreenAvailability) override;
  const Vector<KURL>& Urls() const override;

  // ScriptWrappable implementation.
  bool HasPendingActivity() const final;

  // PausableObject implementation.
  void Pause() override;
  void Unpause() override;
  void ContextDestroyed(ExecutionContext*) override;

  // PageVisibilityObserver implementation.
  void PageVisibilityChanged() override;

  bool value() const;

  DEFINE_ATTRIBUTE_EVENT_LISTENER(change);

  void Trace(blink::Visitor*) override;

 protected:
  // EventTarget implementation.
  void AddedEventListener(const AtomicString& event_type,
                          RegisteredEventListener&) override;

 private:
  // Current state of the PausableObject. It is Active when created. It
  // becomes Suspended when suspend() is called and moves back to Active if
  // resume() is called. It becomes Inactive when stop() is called or at
  // destruction time.
  enum class State : char {
    kActive,
    kSuspended,
    kInactive,
  };

  PresentationAvailability(ExecutionContext*, const WTF::Vector<KURL>&, bool);

  void SetState(State);
  void UpdateListening();

  Vector<KURL> urls_;
  bool value_;
  State state_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PRESENTATION_PRESENTATION_AVAILABILITY_H_

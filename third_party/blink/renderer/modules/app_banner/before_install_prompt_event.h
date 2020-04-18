// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_APP_BANNER_BEFORE_INSTALL_PROMPT_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_APP_BANNER_BEFORE_INSTALL_PROMPT_EVENT_H_

#include <utility>
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/platform/modules/app_banner/app_banner.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_property.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/app_banner/app_banner_prompt_result.h"
#include "third_party/blink/renderer/modules/event_modules.h"

namespace blink {

class BeforeInstallPromptEvent;
class BeforeInstallPromptEventInit;

using UserChoiceProperty =
    ScriptPromiseProperty<Member<BeforeInstallPromptEvent>,
                          AppBannerPromptResult,
                          ToV8UndefinedGenerator>;

class BeforeInstallPromptEvent final
    : public Event,
      public mojom::blink::AppBannerEvent,
      public ActiveScriptWrappable<BeforeInstallPromptEvent>,
      public ContextClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_PRE_FINALIZER(BeforeInstallPromptEvent, Dispose);
  USING_GARBAGE_COLLECTED_MIXIN(BeforeInstallPromptEvent);

 public:
  ~BeforeInstallPromptEvent() override;

  static BeforeInstallPromptEvent* Create(
      const AtomicString& name,
      LocalFrame& frame,
      mojom::blink::AppBannerServicePtr service_ptr,
      mojom::blink::AppBannerEventRequest event_request,
      const Vector<String>& platforms) {
    return new BeforeInstallPromptEvent(name, frame, std::move(service_ptr),
                                        std::move(event_request), platforms);
  }

  static BeforeInstallPromptEvent* Create(
      ExecutionContext* execution_context,
      const AtomicString& name,
      const BeforeInstallPromptEventInit& init) {
    return new BeforeInstallPromptEvent(execution_context, name, init);
  }

  void Dispose();

  Vector<String> platforms() const;
  ScriptPromise userChoice(ScriptState*);
  ScriptPromise prompt(ScriptState*);

  const AtomicString& InterfaceName() const override;
  void preventDefault() override;

  // ScriptWrappable
  bool HasPendingActivity() const override;

  void Trace(blink::Visitor*) override;

 private:
  BeforeInstallPromptEvent(const AtomicString& name,
                           LocalFrame&,
                           mojom::blink::AppBannerServicePtr,
                           mojom::blink::AppBannerEventRequest,
                           const Vector<String>& platforms);
  BeforeInstallPromptEvent(ExecutionContext*,
                           const AtomicString& name,
                           const BeforeInstallPromptEventInit&);

  // mojom::blink::AppBannerEvent methods:
  void BannerAccepted(const String& platform) override;
  void BannerDismissed() override;

  mojom::blink::AppBannerServicePtr banner_service_;
  mojo::Binding<mojom::blink::AppBannerEvent> binding_;
  Vector<String> platforms_;
  Member<UserChoiceProperty> user_choice_;
  bool prompt_called_;
};

DEFINE_TYPE_CASTS(BeforeInstallPromptEvent,
                  Event,
                  event,
                  event->InterfaceName() ==
                      EventNames::BeforeInstallPromptEvent,
                  event.InterfaceName() ==
                      EventNames::BeforeInstallPromptEvent);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_APP_BANNER_BEFORE_INSTALL_PROMPT_EVENT_H_

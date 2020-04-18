// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BATTERY_BATTERY_MANAGER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BATTERY_BATTERY_MANAGER_H_

#include "third_party/blink/renderer/bindings/core/v8/active_script_wrappable.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_property.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/dom/pausable_object.h"
#include "third_party/blink/renderer/core/frame/platform_event_controller.h"
#include "third_party/blink/renderer/modules/battery/battery_status.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class BatteryManager final : public EventTargetWithInlineData,
                             public ActiveScriptWrappable<BatteryManager>,
                             public PausableObject,
                             public PlatformEventController {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(BatteryManager);

 public:
  static BatteryManager* Create(ExecutionContext*);
  ~BatteryManager() override;

  // Returns a promise object that will be resolved with this BatteryManager.
  ScriptPromise StartRequest(ScriptState*);

  // EventTarget implementation.
  const WTF::AtomicString& InterfaceName() const override {
    return EventTargetNames::BatteryManager;
  }
  ExecutionContext* GetExecutionContext() const override {
    return ContextLifecycleObserver::GetExecutionContext();
  }

  bool charging();
  double chargingTime();
  double dischargingTime();
  double level();

  DEFINE_ATTRIBUTE_EVENT_LISTENER(chargingchange);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(chargingtimechange);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(dischargingtimechange);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(levelchange);

  // Inherited from PlatformEventController.
  void DidUpdateData() override;
  void RegisterWithDispatcher() override;
  void UnregisterWithDispatcher() override;
  bool HasLastData() override;

  // PausableObject implementation.
  void Pause() override;
  void Unpause() override;
  void ContextDestroyed(ExecutionContext*) override;

  // ScriptWrappable implementation.
  bool HasPendingActivity() const final;

  void Trace(blink::Visitor*) override;

 private:
  explicit BatteryManager(ExecutionContext*);

  using BatteryProperty = ScriptPromiseProperty<Member<BatteryManager>,
                                                Member<BatteryManager>,
                                                Member<DOMException>>;
  Member<BatteryProperty> battery_property_;
  BatteryStatus battery_status_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BATTERY_BATTERY_MANAGER_H_

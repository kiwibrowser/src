// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BATTERY_NAVIGATOR_BATTERY_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BATTERY_NAVIGATOR_BATTERY_H_

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class BatteryManager;
class Navigator;

class NavigatorBattery final : public GarbageCollected<NavigatorBattery>,
                               public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorBattery);

 public:
  static const char kSupplementName[];

  static NavigatorBattery& From(Navigator&);

  static ScriptPromise getBattery(ScriptState*, Navigator&);
  ScriptPromise getBattery(ScriptState*);

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorBattery(Navigator&);

  Member<BatteryManager> battery_manager_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BATTERY_NAVIGATOR_BATTERY_H_

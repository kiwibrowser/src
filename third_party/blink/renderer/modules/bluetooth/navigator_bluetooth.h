// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BLUETOOTH_NAVIGATOR_BLUETOOTH_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BLUETOOTH_NAVIGATOR_BLUETOOTH_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Bluetooth;
class Navigator;

class NavigatorBluetooth final : public GarbageCollected<NavigatorBluetooth>,
                                 public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorBluetooth);

 public:
  static const char kSupplementName[];

  // Gets, or creates, NavigatorBluetooth supplement on Navigator.
  // See platform/Supplementable.h
  static NavigatorBluetooth& From(Navigator&);

  static Bluetooth* bluetooth(Navigator&);

  // IDL exposed interface:
  Bluetooth* bluetooth();

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorBluetooth(Navigator&);

  Member<Bluetooth> bluetooth_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BLUETOOTH_NAVIGATOR_BLUETOOTH_H_

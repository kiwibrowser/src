// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBUSB_NAVIGATOR_USB_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBUSB_NAVIGATOR_USB_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Navigator;
class USB;

class NavigatorUSB final : public GarbageCollected<NavigatorUSB>,
                           public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorUSB);

 public:
  static const char kSupplementName[];

  // Gets, or creates, NavigatorUSB supplement on Navigator.
  // See platform/Supplementable.h
  static NavigatorUSB& From(Navigator&);

  static USB* usb(Navigator&);
  USB* usb();

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorUSB(Navigator&);

  Member<USB> usb_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBUSB_NAVIGATOR_USB_H_

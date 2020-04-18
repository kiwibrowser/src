// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_ORIENTATION_ABSOLUTE_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_ORIENTATION_ABSOLUTE_CONTROLLER_H_

#include "third_party/blink/renderer/modules/device_orientation/device_orientation_controller.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

class MODULES_EXPORT DeviceOrientationAbsoluteController final
    : public DeviceOrientationController {
 public:
  static const char kSupplementName[];

  ~DeviceOrientationAbsoluteController() override;

  static DeviceOrientationAbsoluteController& From(Document&);

  // Inherited from DeviceSingleWindowEventController.
  void DidAddEventListener(LocalDOMWindow*,
                           const AtomicString& event_type) override;

  void Trace(blink::Visitor*) override;

 private:
  explicit DeviceOrientationAbsoluteController(Document&);

  // Inherited from DeviceOrientationController.
  DeviceOrientationDispatcher& DispatcherInstance() const override;
  const AtomicString& EventTypeName() const override;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_ORIENTATION_ABSOLUTE_CONTROLLER_H_

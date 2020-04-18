// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/device_orientation/device_orientation_absolute_controller.h"

#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/modules/device_orientation/device_orientation_dispatcher.h"

namespace blink {

DeviceOrientationAbsoluteController::DeviceOrientationAbsoluteController(
    Document& document)
    : DeviceOrientationController(document) {}

DeviceOrientationAbsoluteController::~DeviceOrientationAbsoluteController() =
    default;

const char DeviceOrientationAbsoluteController::kSupplementName[] =
    "DeviceOrientationAbsoluteController";

DeviceOrientationAbsoluteController& DeviceOrientationAbsoluteController::From(
    Document& document) {
  DeviceOrientationAbsoluteController* controller =
      Supplement<Document>::From<DeviceOrientationAbsoluteController>(document);
  if (!controller) {
    controller = new DeviceOrientationAbsoluteController(document);
    Supplement<Document>::ProvideTo(document, controller);
  }
  return *controller;
}

void DeviceOrientationAbsoluteController::DidAddEventListener(
    LocalDOMWindow* window,
    const AtomicString& event_type) {
  if (event_type != EventTypeName())
    return;

  LocalFrame* frame = GetDocument().GetFrame();
  if (frame) {
    if (GetDocument().IsSecureContext()) {
      UseCounter::Count(frame,
                        WebFeature::kDeviceOrientationAbsoluteSecureOrigin);
    } else {
      Deprecation::CountDeprecation(
          frame, WebFeature::kDeviceOrientationAbsoluteInsecureOrigin);
      // TODO: add rappor logging of insecure origins as in
      // DeviceOrientationController.
      if (frame->GetSettings()->GetStrictPowerfulFeatureRestrictions())
        return;
    }
  }

  if (!has_event_listener_) {
    // TODO: add rappor url logging as in DeviceOrientationController.

    if (!CheckPolicyFeatures({mojom::FeaturePolicyFeature::kAccelerometer,
                              mojom::FeaturePolicyFeature::kGyroscope,
                              mojom::FeaturePolicyFeature::kMagnetometer})) {
      LogToConsolePolicyFeaturesDisabled(frame, EventTypeName());
      return;
    }
  }

  DeviceSingleWindowEventController::DidAddEventListener(window, event_type);
}

DeviceOrientationDispatcher&
DeviceOrientationAbsoluteController::DispatcherInstance() const {
  return DeviceOrientationDispatcher::Instance(true);
}

const AtomicString& DeviceOrientationAbsoluteController::EventTypeName() const {
  return EventTypeNames::deviceorientationabsolute;
}

void DeviceOrientationAbsoluteController::Trace(blink::Visitor* visitor) {
  DeviceOrientationController::Trace(visitor);
}

}  // namespace blink

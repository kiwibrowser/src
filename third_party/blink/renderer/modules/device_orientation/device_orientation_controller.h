// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_ORIENTATION_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_ORIENTATION_CONTROLLER_H_

#include "third_party/blink/renderer/core/frame/device_single_window_event_controller.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

class DeviceOrientationData;
class DeviceOrientationDispatcher;
class Event;

class MODULES_EXPORT DeviceOrientationController
    : public DeviceSingleWindowEventController,
      public Supplement<Document> {
  USING_GARBAGE_COLLECTED_MIXIN(DeviceOrientationController);

 public:
  static const char kSupplementName[];

  ~DeviceOrientationController() override;

  static DeviceOrientationController& From(Document&);

  // Inherited from DeviceSingleWindowEventController.
  void DidUpdateData() override;
  void DidAddEventListener(LocalDOMWindow*,
                           const AtomicString& event_type) override;

  void SetOverride(DeviceOrientationData*);
  void ClearOverride();

  void Trace(blink::Visitor*) override;

  static void LogToConsolePolicyFeaturesDisabled(
      LocalFrame*,
      const AtomicString& event_name);

 protected:
  explicit DeviceOrientationController(Document&);

  virtual DeviceOrientationDispatcher& DispatcherInstance() const;

 private:
  // Inherited from DeviceEventControllerBase.
  void RegisterWithDispatcher() override;
  void UnregisterWithDispatcher() override;
  bool HasLastData() override;

  // Inherited from DeviceSingleWindowEventController.
  Event* LastEvent() const override;
  const AtomicString& EventTypeName() const override;
  bool IsNullEvent(Event*) const override;

  DeviceOrientationData* LastData() const;

  Member<DeviceOrientationData> override_orientation_data_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_ORIENTATION_CONTROLLER_H_

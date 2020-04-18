// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_OZONE_GAMEPAD_EVENT_CONVERTER_EVDEV_H_
#define UI_EVENTS_OZONE_GAMEPAD_EVENT_CONVERTER_EVDEV_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "ui/events/devices/input_device.h"
#include "ui/events/event.h"
#include "ui/events/ozone/evdev/event_converter_evdev.h"
#include "ui/events/ozone/evdev/event_device_info.h"
#include "ui/events/ozone/evdev/events_ozone_evdev_export.h"
#include "ui/events/ozone/gamepad/gamepad_mapping.h"
#include "ui/events/ozone/gamepad/webgamepad_constants.h"

struct input_event;

namespace ui {

class DeviceEventDispatcherEvdev;

class EVENTS_OZONE_EVDEV_EXPORT GamepadEventConverterEvdev
    : public EventConverterEvdev {
 public:
  GamepadEventConverterEvdev(base::ScopedFD fd,
                             base::FilePath path,
                             int id,
                             const EventDeviceInfo& info,
                             DeviceEventDispatcherEvdev* dispatcher);

  ~GamepadEventConverterEvdev() override;

  // EventConverterEvdev:
  void OnFileCanReadWithoutBlocking(int fd) override;
  bool HasGamepad() const override;
  void OnDisabled() override;

  // This function processes one input_event from evdev.
  void ProcessEvent(const struct input_event& input);

 private:
  // This function processes EV_KEY event from gamepad device.
  void ProcessEvdevKey(uint16_t code,
                       int value,
                       const base::TimeTicks& timestamp);

  // This function processes EV_ABS event from gamepad device.
  void ProcessEvdevAbs(uint16_t code,
                       int value,
                       const base::TimeTicks& timestamp);

  // This function releases all the keys and resets all the axises.
  void ResetGamepad();

  // This function reads current gamepad status and resyncs the gamepad.
  void ResyncGamepad();

  void OnButtonChange(unsigned int code,
                      double value,
                      const base::TimeTicks& timestamp);
  void OnAbsChange(unsigned int code,
                   double value,
                   const base::TimeTicks& timestamp);
  void OnSync(const base::TimeTicks& timestamp);

  // Sometimes, we want to drop abs values, when we do so, we no longer want to
  // send gamepad frame event when we see next sync. This flag is set to false
  // when each frame is sent. It is set to true when Btn or Abs event is sent.
  bool will_send_frame_;

  // Internal representation of axis information.

  class Axis {
   public:
    Axis();
    Axis(const input_absinfo& abs_info,
         GamepadEventType mapped_type,
         uint16_t mapped_code);

    bool MapValue(int value, double* mapped_value);

    GamepadEventType mapped_type();

    uint16_t mapped_code();

   private:
    bool ValueChangeSignificantly(double new_value);
    double last_value_;
    double scale_;
    double offset_;
    double scaled_fuzz_;
    double scaled_flat_;
    GamepadEventType mapped_type_;
    uint16_t mapped_code_;
  };

  Axis axes_[ABS_CNT];

  // These values keeps the state of previous hat.
  bool last_hat_left_press_;
  bool last_hat_right_press_;
  bool last_hat_up_press_;
  bool last_hat_down_press_;

  std::unique_ptr<GamepadMapper> mapper_;

  // Input device file descriptor.
  base::ScopedFD input_device_fd_;

  // Callbacks for dispatching events.
  DeviceEventDispatcherEvdev* dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(GamepadEventConverterEvdev);
};

}  // namespace ui

#endif  // UI_EVENTS_OZONE_GAMEPAD_EVENT_CONVERTER_EVDEV_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/gamepad_event_converter_evdev.h"

#include <errno.h>
#include <linux/input.h>
#include <stddef.h>

#include "base/trace_event/trace_event.h"
#include "ui/events/event_utils.h"
#include "ui/events/ozone/evdev/device_event_dispatcher_evdev.h"
#include "ui/events/ozone/gamepad/gamepad_event.h"
#include "ui/events/ozone/gamepad/gamepad_provider_ozone.h"
#include "ui/events/ozone/gamepad/webgamepad_constants.h"

namespace {
constexpr double kHatThreshold = 0.5;
}

namespace ui {

GamepadEventConverterEvdev::Axis::Axis()
    : last_value_(0.),
      scale_(0.),
      offset_(0.),
      scaled_fuzz_(0.),
      scaled_flat_(0.),
      mapped_type_(GamepadEventType::BUTTON),
      mapped_code_(0.) {}

GamepadEventConverterEvdev::Axis::Axis(const input_absinfo& abs_info,
                                       GamepadEventType mapped_type,
                                       uint16_t mapped_code)
    : mapped_type_(mapped_type), mapped_code_(mapped_code) {
  double mapped_abs_min = kWebGamepadJoystickMin;
  double mapped_abs_max = kWebGamepadJoystickMax;
  // If the mapped event is a trigger, we set the min and max to trigger
  // min/max.
  if (mapped_code == WG_BUTTON_LT || mapped_code == WG_BUTTON_RT) {
    mapped_abs_min = kWebGamepadTriggerMin;
    mapped_abs_max = kWebGamepadTriggerMax;
  }

  scale_ =
      (mapped_abs_max - mapped_abs_min) / (abs_info.maximum - abs_info.minimum);
  offset_ = (abs_info.maximum + abs_info.minimum) /
            (mapped_abs_max - mapped_abs_min) * scale_ * mapped_abs_min;

  scaled_flat_ = abs_info.flat * scale_;
  scaled_fuzz_ = abs_info.fuzz * scale_;
  double tmp;
  // Map the current value and it will be set to value_.
  MapValue(abs_info.value, &tmp);
}

bool GamepadEventConverterEvdev::Axis::MapValue(int value,
                                                double* mapped_value) {
  *mapped_value = value * scale_ + offset_;
  // As the definition of linux input_absinfo.flat, value within the range of
  // flat should be seen as zero.
  if ((*mapped_value < scaled_flat_) && (*mapped_value > -scaled_flat_)) {
    *mapped_value = 0.0;
    // We always send out flat.
    last_value_ = 0.0;
    return true;
  }
  return ValueChangeSignificantly(*mapped_value);
}

GamepadEventType GamepadEventConverterEvdev::Axis::mapped_type() {
  return mapped_type_;
}

uint16_t GamepadEventConverterEvdev::Axis::mapped_code() {
  return mapped_code_;
}

bool GamepadEventConverterEvdev::Axis::ValueChangeSignificantly(
    double new_value) {
  // To remove noise, the value must change at least by fuzz.
  if (new_value >= last_value_ - scaled_fuzz_ &&
      new_value <= last_value_ + scaled_fuzz_) {
    return false;
  }
  last_value_ = new_value;
  return true;
}

GamepadEventConverterEvdev::GamepadEventConverterEvdev(
    base::ScopedFD fd,
    base::FilePath path,
    int id,
    const EventDeviceInfo& devinfo,
    DeviceEventDispatcherEvdev* dispatcher)
    : EventConverterEvdev(fd.get(),
                          path,
                          id,
                          devinfo.device_type(),
                          devinfo.name(),
                          devinfo.phys(),
                          devinfo.vendor_id(),
                          devinfo.product_id()),
      will_send_frame_(false),
      last_hat_left_press_(false),
      last_hat_right_press_(false),
      last_hat_up_press_(false),
      last_hat_down_press_(false),
      mapper_(GetGamepadMapper(devinfo)),
      input_device_fd_(std::move(fd)),
      dispatcher_(dispatcher) {
  input_absinfo abs_info;
  GamepadEventType mapped_type;
  uint16_t mapped_code;
  // In order to map gamepad, we have to save the abs_info from device_info
  // and get the gamepad_mapping.
  for (int code = 0; code < ABS_CNT; ++code) {
    abs_info = devinfo.GetAbsInfoByCode(code);
    if (devinfo.HasAbsEvent(code)) {
      // If fuzz was reported as zero, it will be set to flat * 0.25f. It is
      // the same thing done in Android InputReader.cpp. See:
      // frameworks/native/services/inputflinger/InputReader.cpp line 6988 for
      // more details.
      if (abs_info.fuzz == 0) {
        abs_info.fuzz = abs_info.flat * 0.25f;
      }
      mapper_->Map(EV_ABS, code, &mapped_type, &mapped_code);
      axes_[code] = Axis(abs_info, mapped_type, mapped_code);
    }
  }
}

GamepadEventConverterEvdev::~GamepadEventConverterEvdev() {
  DCHECK(!IsEnabled());
}

bool GamepadEventConverterEvdev::HasGamepad() const {
  return true;
}

void GamepadEventConverterEvdev::OnFileCanReadWithoutBlocking(int fd) {
  TRACE_EVENT1("evdev",
               "GamepadEventConverterEvdev::OnFileCanReadWithoutBlocking", "fd",
               fd);
  while (true) {
    input_event input;
    ssize_t read_size = read(fd, &input, sizeof(input));
    if (read_size != sizeof(input)) {
      if (errno == EINTR || errno == EAGAIN)
        return;
      if (errno != ENODEV)
        PLOG(ERROR) << "error reading device " << path_.value();
      Stop();
      return;
    }

    if (!IsEnabled())
      return;

    ProcessEvent(input);
  }
}
void GamepadEventConverterEvdev::OnDisabled() {
  ResetGamepad();
}

void GamepadEventConverterEvdev::ProcessEvent(const input_event& evdev_ev) {
  base::TimeTicks timestamp = TimeTicksFromInputEvent(evdev_ev);
  // We may have missed Gamepad releases. Reset everything.
  // If the event is sync, we send a frame.
  if (evdev_ev.type == EV_SYN) {
    if (evdev_ev.code == SYN_DROPPED) {
      LOG(WARNING) << "kernel dropped input events";
      ResyncGamepad();
    } else if (evdev_ev.code == SYN_REPORT) {
      OnSync(timestamp);
    }
  } else if (evdev_ev.type == EV_KEY) {
    ProcessEvdevKey(evdev_ev.code, evdev_ev.value, timestamp);
  } else if (evdev_ev.type == EV_ABS) {
    ProcessEvdevAbs(evdev_ev.code, evdev_ev.value, timestamp);
  }
}

void GamepadEventConverterEvdev::ProcessEvdevKey(
    uint16_t code,
    int value,
    const base::TimeTicks& timestamp) {
  GamepadEventType mapped_type;
  uint16_t mapped_code;

  bool found_map = mapper_->Map(EV_KEY, code, &mapped_type, &mapped_code);

  // If we cannot find a map for this event, it will be discarded.
  if (!found_map) {
    return;
  }

  // If it's btn -> btn mapping, we can send the event and return now.
  OnButtonChange(mapped_code, value, timestamp);
}

void GamepadEventConverterEvdev::ProcessEvdevAbs(
    uint16_t code,
    int value,
    const base::TimeTicks& timestamp) {
  GamepadEventType mapped_type;
  uint16_t mapped_code;

  double mapped_abs_value;
  Axis& axis = axes_[code];
  mapped_type = axis.mapped_type();
  mapped_code = axis.mapped_code();

  if (!axis.MapValue(value, &mapped_abs_value)) {
    return;
  }

  // If the mapped type is abs, we can send it now.
  if (mapped_type == GamepadEventType::AXIS) {
    OnAbsChange(mapped_code, mapped_abs_value, timestamp);
    return;
  }

  // We need to map HAT to DPAD depend on the state of the axis.
  if (mapped_code == kHAT_X) {
    bool hat_left_press = (mapped_abs_value < -kHatThreshold);
    bool hat_right_press = (mapped_abs_value > kHatThreshold);
    if (hat_left_press != last_hat_left_press_) {
      OnButtonChange(WG_BUTTON_DPAD_LEFT, hat_left_press, timestamp);
      last_hat_left_press_ = hat_left_press;
    }

    if (hat_right_press != last_hat_right_press_) {
      OnButtonChange(WG_BUTTON_DPAD_RIGHT, hat_right_press, timestamp);
      last_hat_right_press_ = hat_right_press;
    }
  } else if (mapped_code == kHAT_Y) {
    bool hat_up_press = (mapped_abs_value < -kHatThreshold);
    bool hat_down_press = (mapped_abs_value > kHatThreshold);
    if (hat_up_press != last_hat_up_press_) {
      OnButtonChange(WG_BUTTON_DPAD_UP, hat_up_press, timestamp);
      last_hat_up_press_ = hat_up_press;
    }

    if (hat_down_press != last_hat_down_press_) {
      OnButtonChange(WG_BUTTON_DPAD_DOWN, hat_down_press, timestamp);
      last_hat_down_press_ = hat_down_press;
    }
  } else {
    OnButtonChange(mapped_code, mapped_abs_value, timestamp);
  }
}

void GamepadEventConverterEvdev::ResetGamepad() {
  base::TimeTicks timestamp = ui::EventTimeForNow();
  for (int btn_code = 0; btn_code < WG_BUTTON_COUNT; ++btn_code) {
    OnButtonChange(btn_code, 0, timestamp);
  }

  for (int abs_code = 0; abs_code < WG_ABS_COUNT; ++abs_code) {
    OnAbsChange(abs_code, 0, timestamp);
  }
  OnSync(timestamp);
}

void GamepadEventConverterEvdev::ResyncGamepad() {
  base::TimeTicks timestamp = ui::EventTimeForNow();
  // Reset all the buttons to 0.
  for (int btn_code = 0; btn_code < WG_BUTTON_COUNT; ++btn_code) {
    OnButtonChange(btn_code, 0, timestamp);
  }
  // Read the state of all axis.
  EventDeviceInfo info;
  if (!info.Initialize(fd_, path_)) {
    LOG(ERROR) << "Failed to synchronize state for gamepad device: "
               << path_.value();
    Stop();
    return;
  }
  for (int code = 0; code < ABS_CNT; ++code) {
    if (info.HasAbsEvent(code)) {
      ProcessEvdevAbs(code, info.GetAbsValue(code), timestamp);
    }
  }
  OnSync(timestamp);
}

void GamepadEventConverterEvdev::OnButtonChange(
    unsigned int code,
    double value,
    const base::TimeTicks& timestamp) {
  GamepadEvent event(input_device_.id, GamepadEventType::BUTTON, code, value,
                     timestamp);
  dispatcher_->DispatchGamepadEvent(event);
  will_send_frame_ = true;
}

void GamepadEventConverterEvdev::OnAbsChange(unsigned int code,
                                             double value,
                                             const base::TimeTicks& timestamp) {
  GamepadEvent event(input_device_.id, GamepadEventType::AXIS, code, value,
                     timestamp);
  dispatcher_->DispatchGamepadEvent(event);
  will_send_frame_ = true;
}

void GamepadEventConverterEvdev::OnSync(const base::TimeTicks& timestamp) {
  if (will_send_frame_) {
    GamepadEvent event(input_device_.id, GamepadEventType::FRAME, 0, 0,
                       timestamp);
    dispatcher_->DispatchGamepadEvent(event);
    will_send_frame_ = false;
  }
}
}  //  namespace ui

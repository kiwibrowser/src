// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_WAYLAND_KEYBOARD_H_
#define UI_OZONE_PLATFORM_WAYLAND_WAYLAND_KEYBOARD_H_

#include <wayland-client.h>

#include "ui/events/event_modifiers.h"
#include "ui/events/ozone/evdev/event_dispatch_callback.h"
#include "ui/events/ozone/keyboard/event_auto_repeat_handler.h"
#include "ui/ozone/platform/wayland/wayland_object.h"

namespace ui {

class WaylandConnection;

class WaylandKeyboard : public EventAutoRepeatHandler::Delegate {
 public:
  WaylandKeyboard(wl_keyboard* keyboard, const EventDispatchCallback& callback);
  virtual ~WaylandKeyboard();

  void set_connection(WaylandConnection* connection) {
    connection_ = connection;
  }

  int modifiers() { return event_modifiers_.GetModifierFlags(); }

 private:
  // wl_keyboard_listener
  static void Keymap(void* data,
                     wl_keyboard* obj,
                     uint32_t format,
                     int32_t fd,
                     uint32_t size);
  static void Enter(void* data,
                    wl_keyboard* obj,
                    uint32_t serial,
                    wl_surface* surface,
                    wl_array* keys);
  static void Leave(void* data,
                    wl_keyboard* obj,
                    uint32_t serial,
                    wl_surface* surface);
  static void Key(void* data,
                  wl_keyboard* obj,
                  uint32_t serial,
                  uint32_t time,
                  uint32_t key,
                  uint32_t state);
  static void Modifiers(void* data,
                        wl_keyboard* obj,
                        uint32_t serial,
                        uint32_t mods_depressed,
                        uint32_t mods_latched,
                        uint32_t mods_locked,
                        uint32_t group);
  static void RepeatInfo(void* data,
                         wl_keyboard* obj,
                         int32_t rate,
                         int32_t delay);

  static void SyncCallback(void* data, struct wl_callback* cb, uint32_t time);

  void UpdateModifier(int modifier_flag, bool down);

  // EventAutoRepeatHandler::Delegate
  void FlushInput(base::OnceClosure closure) override;
  void DispatchKey(unsigned int key,
                   bool down,
                   bool repeat,
                   base::TimeTicks timestamp,
                   int device_id) override;

  WaylandConnection* connection_ = nullptr;
  wl::Object<wl_keyboard> obj_;
  EventDispatchCallback callback_;
  EventModifiers event_modifiers_;

  // Key repeat handler.
  static const wl_callback_listener callback_listener_;
  EventAutoRepeatHandler auto_repeat_handler_;
  base::OnceClosure auto_repeat_closure_;
  wl::Object<wl_callback> sync_callback_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_WAYLAND_KEYBOARD_H_

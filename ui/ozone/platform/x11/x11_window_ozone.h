// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_X11_X11_WINDOW_OZONE_H_
#define UI_OZONE_PLATFORM_X11_X11_WINDOW_OZONE_H_

#include "base/macros.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/events/platform/x11/x11_event_source_libevent.h"
#include "ui/platform_window/x11/x11_window_base.h"

namespace ui {

class X11WindowManagerOzone;

// PlatformWindow implementation for X11 Ozone. PlatformEvents are ui::Events.
class X11WindowOzone : public X11WindowBase,
                       public PlatformEventDispatcher,
                       public XEventDispatcher {
 public:
  X11WindowOzone(X11WindowManagerOzone* window_manager,
                 PlatformWindowDelegate* delegate,
                 const gfx::Rect& bounds);
  ~X11WindowOzone() override;

  // Called by |window_manager_| once capture is set to another X11WindowOzone.
  void OnLostCapture();

  // PlatformWindow:
  void PrepareForShutdown() override;
  void SetCapture() override;
  void ReleaseCapture() override;
  void SetCursor(PlatformCursor cursor) override;

  // XEventDispatcher:
  void CheckCanDispatchNextPlatformEvent(XEvent* xev) override;
  void PlatformEventDispatchFinished() override;
  PlatformEventDispatcher* GetPlatformEventDispatcher() override;
  bool DispatchXEvent(XEvent* event) override;

 private:
  // PlatformEventDispatcher:
  bool CanDispatchEvent(const PlatformEvent& event) override;
  uint32_t DispatchEvent(const PlatformEvent& event) override;

  X11WindowManagerOzone* window_manager_;

  // Tells if this dispatcher can process next translated event based on a
  // previous check in ::CheckCanDispatchNextPlatformEvent based on a XID
  // target.
  bool handle_next_event_ = false;

  DISALLOW_COPY_AND_ASSIGN(X11WindowOzone);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_X11_X11_WINDOW_OZONE_H_

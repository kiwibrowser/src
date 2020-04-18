// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_PLATFORM_WINDOW_X11_X11_WINDOW_H_
#define UI_PLATFORM_WINDOW_X11_X11_WINDOW_H_

#include "base/macros.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/platform_window/x11/x11_window_base.h"
#include "ui/platform_window/x11/x11_window_export.h"

namespace ui {

// PlatformWindow implementation for X11. PlatformEvents are XEvents.
class X11_WINDOW_EXPORT X11Window : public X11WindowBase,
                                    public PlatformEventDispatcher {
 public:
  X11Window(PlatformWindowDelegate* delegate, const gfx::Rect& bounds);
  ~X11Window() override;

  // PlatformWindow:
  void PrepareForShutdown() override;
  void SetCursor(PlatformCursor cursor) override;

 private:
  void ProcessXInput2Event(XEvent* xev);

  // PlatformEventDispatcher:
  bool CanDispatchEvent(const PlatformEvent& event) override;
  uint32_t DispatchEvent(const PlatformEvent& event) override;

  DISALLOW_COPY_AND_ASSIGN(X11Window);
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_X11_X11_WINDOW_H_

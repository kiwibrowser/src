// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_X11_DESKTOP_HANDLER_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_X11_DESKTOP_HANDLER_H_

#include <stdint.h>

#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "ui/aura/env_observer.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/events/platform/x11/x11_event_source.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/views/views_export.h"
#include "ui/views/widget/desktop_aura/x11_desktop_handler_observer.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

namespace ui {
class XScopedEventSelector;
}

namespace views {

// A singleton that owns global objects related to the desktop and listens for
// X11 events on the X11 root window. Destroys itself when aura::Env is
// deleted.
class VIEWS_EXPORT X11DesktopHandler : public ui::PlatformEventDispatcher,
                                       public aura::EnvObserver {
 public:
  // Returns the singleton handler.  Creates one if one has not
  // already been created.
  static X11DesktopHandler* get();

  // Returns the singleton handler, or nullptr if one has not already
  // been created.
  static X11DesktopHandler* get_dont_create();

  // Adds/removes X11DesktopHandlerObservers.
  void AddObserver(X11DesktopHandlerObserver* observer);
  void RemoveObserver(X11DesktopHandlerObserver* observer);

  // Gets the current workspace ID.
  std::string GetWorkspace();

  // ui::PlatformEventDispatcher
  bool CanDispatchEvent(const ui::PlatformEvent& event) override;
  uint32_t DispatchEvent(const ui::PlatformEvent& event) override;

  // Overridden from aura::EnvObserver:
  void OnWindowInitialized(aura::Window* window) override;
  void OnWillDestroyEnv() override;

 private:
  X11DesktopHandler();
  ~X11DesktopHandler() override;

  // Called when |window| has been created or destroyed. |window| may not be
  // managed by Chrome.
  void OnWindowCreatedOrDestroyed(int event_type, XID window);

  // Makes a round trip to the X server to get the current workspace.
  bool UpdateWorkspace();

  // The display and the native X window hosting the root window.
  XDisplay* xdisplay_;

  // The native root window.
  ::Window x_root_window_;

  // Events selected on x_root_window_.
  std::unique_ptr<ui::XScopedEventSelector> x_root_window_events_;

  base::ObserverList<X11DesktopHandlerObserver> observers_;

  std::string workspace_;

  DISALLOW_COPY_AND_ASSIGN(X11DesktopHandler);
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_X11_DESKTOP_HANDLER_H_

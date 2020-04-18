// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_X11_WINDOW_EVENT_FILTER_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_X11_WINDOW_EVENT_FILTER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/events/event_handler.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/views/views_export.h"
#include "ui/views/widget/desktop_aura/window_event_filter.h"

namespace gfx {
class Point;
}

namespace views {
class DesktopWindowTreeHost;

// An EventFilter that sets properties on X11 windows.
class VIEWS_EXPORT X11WindowEventFilter : public WindowEventFilter {
 public:
  explicit X11WindowEventFilter(DesktopWindowTreeHost* window_tree_host);
  ~X11WindowEventFilter() override;

 private:
  // WindowEventFilter override:
  void MaybeDispatchHostWindowDragMovement(int hittest,
                                           ui::MouseEvent* event) override;
  void LowerWindow() override;

  bool DispatchHostWindowDragMovement(int hittest,
                                      const gfx::Point& screen_location);

  // The display and the native X window hosting the root window.
  XDisplay* xdisplay_;
  ::Window xwindow_;

  // The native root window.
  ::Window x_root_window_;

  DISALLOW_COPY_AND_ASSIGN(X11WindowEventFilter);
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_X11_WINDOW_EVENT_FILTER_H_

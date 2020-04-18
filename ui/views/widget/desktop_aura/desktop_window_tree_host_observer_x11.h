// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_OBSERVER_X11_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_OBSERVER_X11_H_

#include "ui/views/views_export.h"

namespace views {

// Allows for the observation of lower level window events.
class VIEWS_EXPORT DesktopWindowTreeHostObserverX11 {
 public:
  virtual ~DesktopWindowTreeHostObserverX11() {}

  // Called after we receive a MapNotify event (the X11 server has allocated
  // resources for it).
  virtual void OnWindowMapped(unsigned long xid) = 0;

  // Called after we receive an UnmapNotify event (the X11 server has freed
  // resources for it).
  virtual void OnWindowUnmapped(unsigned long xid) = 0;
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_OBSERVER_X11_H_


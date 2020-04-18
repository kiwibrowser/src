// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_PANELS_PANEL_WINDOW_EVENT_HANDLER_H_
#define ASH_WM_PANELS_PANEL_WINDOW_EVENT_HANDLER_H_

#include "ui/events/event_handler.h"

#include "base/macros.h"

namespace ash {

// PanelWindowEventHandler minimizes panels when the user double clicks or
// double taps on the panel header.
class PanelWindowEventHandler : public ui::EventHandler {
 public:
  PanelWindowEventHandler();
  ~PanelWindowEventHandler() override;

  // ui::EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PanelWindowEventHandler);
};

}  // namespace ash

#endif  // ASH_WM_PANELS_PANEL_WINDOW_EVENT_HANDLER_H_

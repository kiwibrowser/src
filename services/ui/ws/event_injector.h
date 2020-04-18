// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_EVENT_INJECTOR_H_
#define SERVICES_UI_WS_EVENT_INJECTOR_H_

#include "services/ui/public/interfaces/event_injector.mojom.h"

namespace ui {
namespace ws {

class Display;
class WindowServer;

// See description in mojom for details on this.
class EventInjector : public mojom::EventInjector {
 public:
  explicit EventInjector(WindowServer* server);
  ~EventInjector() override;

 private:
  // Adjusts the location as necessary of |event|. |display| is the display
  // the event is targetted at.
  void AdjustEventLocationForPixelLayout(Display* display,
                                         ui::LocatedEvent* event);

  // mojom::EventInjector:
  void InjectEvent(int64_t display_id,
                   std::unique_ptr<ui::Event> event,
                   InjectEventCallback cb) override;

  WindowServer* window_server_;

  DISALLOW_COPY_AND_ASSIGN(EventInjector);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_EVENT_INJECTOR_H_

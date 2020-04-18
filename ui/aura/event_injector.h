// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_EVENT_INJECTOR_H_
#define UI_AURA_EVENT_INJECTOR_H_

#include "services/ui/public/interfaces/event_injector.mojom.h"
#include "ui/aura/aura_export.h"

namespace ui {
class Event;
struct EventDispatchDetails;
}

namespace aura {

class WindowTreeHost;

// Used to inject events into the system. In LOCAL mode, it directly injects
// events into the WindowTreeHost, but in MUS mode, it injects events into the
// window-server (over the mojom API).
class AURA_EXPORT EventInjector {
 public:
  EventInjector();
  ~EventInjector();

  ui::EventDispatchDetails Inject(WindowTreeHost* host, ui::Event* event);

 private:
  ui::mojom::EventInjectorPtr event_injector_;

  DISALLOW_COPY_AND_ASSIGN(EventInjector);
};

}  // namespace aura

#endif  // UI_AURA_EVENT_INJECTOR_H_

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_PLATFORM_EVENT_SOURCE_MUS_OZONE_H_
#define UI_AURA_MUS_PLATFORM_EVENT_SOURCE_MUS_OZONE_H_

#include "ui/events/platform/platform_event_source.h"

namespace ui {
class Event;
}

namespace aura {

// PlatformEventSource implementation for mus with ozone. WindowTreeClient owns
// and installs this. WindowTreeClient calls this to notify observers as
// necessary.
class PlatformEventSourceMus : public ui::PlatformEventSource {
 public:
  PlatformEventSourceMus();
  ~PlatformEventSourceMus() override;

  // These two functions are called from WindowTreeClient before/after
  // dispatching events. They forward to observers.
  void OnWillProcessEvent(ui::Event* event);
  void OnDidProcessEvent(ui::Event* event);

 private:
  DISALLOW_COPY_AND_ASSIGN(PlatformEventSourceMus);
};

}  // namespace aura

#endif  // UI_AURA_MUS_PLATFORM_EVENT_SOURCE_MUS_OZONE_H_

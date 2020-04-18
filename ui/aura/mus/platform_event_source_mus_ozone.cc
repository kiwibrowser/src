// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/platform_event_source_mus_ozone.h"

#include "ui/events/event.h"
#include "ui/events/platform/platform_event_observer.h"

namespace aura {

PlatformEventSourceMus::PlatformEventSourceMus() = default;

PlatformEventSourceMus::~PlatformEventSourceMus() = default;

void PlatformEventSourceMus::OnWillProcessEvent(ui::Event* event) {
  for (ui::PlatformEventObserver& observer : observers())
    observer.WillProcessEvent(event);
}

void PlatformEventSourceMus::OnDidProcessEvent(ui::Event* event) {
  for (ui::PlatformEventObserver& observer : observers())
    observer.DidProcessEvent(event);
}

}  // namespace aura

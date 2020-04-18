// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_MUS_MOUSE_LOCATION_UPDATER_H_
#define UI_AURA_MUS_MUS_MOUSE_LOCATION_UPDATER_H_

#include "base/macros.h"
#include "base/run_loop.h"

namespace ui {
class Event;
}

namespace aura {

// MusMouseLocationUpdater is responsible for updating
// Env::last_mouse_location(), as well as determining when
// Env::last_mouse_location() should use
// WindowTreeClient::GetCursorScreenPoint(). While processing an event
// Env uses the value from the current event, otherwise Env uses
// WindowTreeClient::GetCursorScreenPoint(). If a nested run loop is
// started while processing an event Env uses GetCursorScreenPoint().
class MusMouseLocationUpdater : public base::RunLoop::NestingObserver {
 public:
  MusMouseLocationUpdater();
  ~MusMouseLocationUpdater() override;

  // Called from WindowEventDispatcher on starting/stopping processing of
  // events.
  void OnEventProcessingStarted(const ui::Event& event);
  void OnEventProcessingFinished();

 private:
  // Called to switch from using last event location to current cursor screen
  // location.
  void UseCursorScreenPoint();

  // base::RunLoop::NestingObserver:
  void OnBeginNestedRunLoop() override;

  // Set to true while processing a valid mouse event.
  bool is_processing_trigger_event_ = false;

  DISALLOW_COPY_AND_ASSIGN(MusMouseLocationUpdater);
};

}  // namespace aura

#endif  // UI_AURA_MUS_MUS_MOUSE_LOCATION_UPDATER_H_

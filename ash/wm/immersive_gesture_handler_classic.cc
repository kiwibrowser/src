// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/immersive_gesture_handler_classic.h"

#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/shell.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/window.h"
#include "ui/events/event.h"

namespace ash {
namespace {

// Returns the location of |event| in screen coordinates.
gfx::Point GetEventLocationInScreen(const ui::LocatedEvent& event) {
  gfx::Point location_in_screen = event.location();
  aura::Window* target = static_cast<aura::Window*>(event.target());
  aura::client::ScreenPositionClient* screen_position_client =
      aura::client::GetScreenPositionClient(target->GetRootWindow());
  screen_position_client->ConvertPointToScreen(target, &location_in_screen);
  return location_in_screen;
}

}  // namespace

ImmersiveGestureHandlerClassic::ImmersiveGestureHandlerClassic(
    ImmersiveFullscreenController* controller)
    : immersive_fullscreen_controller_(controller) {
  Shell::Get()->AddPreTargetHandler(this);
}

ImmersiveGestureHandlerClassic::~ImmersiveGestureHandlerClassic() {
  Shell::Get()->RemovePreTargetHandler(this);
}

void ImmersiveGestureHandlerClassic::OnGestureEvent(ui::GestureEvent* event) {
  immersive_fullscreen_controller_->OnGestureEvent(
      event, GetEventLocationInScreen(*event));
}

}  // namespace ash

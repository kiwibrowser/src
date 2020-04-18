// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/window_finder.h"

// The direct usage of //ash/ is fine here, as this code is only executed in
// classic ash. See //c/b/u/v/tabs/window_finder_chromeos.h.
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/wm/root_window_finder.h"  // mash-ok
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/wm/core/window_util.h"

namespace {

gfx::NativeWindow GetLocalProcessWindowAtPointImpl(
    const gfx::Point& screen_point,
    const std::set<gfx::NativeWindow>& ignore,
    gfx::NativeWindow window) {
  if (ignore.find(window) != ignore.end())
    return NULL;

  if (!window->IsVisible())
    return NULL;

  if (window->id() == ash::kShellWindowId_PhantomWindow ||
      window->id() == ash::kShellWindowId_OverlayContainer ||
      window->id() == ash::kShellWindowId_MouseCursorContainer)
    return NULL;

  if (window->layer()->type() == ui::LAYER_TEXTURED) {
    // Returns the window that has visible layer and can hit the
    // |screen_point|, because we want to detach the tab as soon as
    // the dragging mouse moved over to the window that can hide the
    // moving tab.
    aura::client::ScreenPositionClient* client =
        aura::client::GetScreenPositionClient(window->GetRootWindow());
    gfx::Point local_point = screen_point;
    client->ConvertPointFromScreen(window, &local_point);
    return window->GetEventHandlerForPoint(local_point) ? window : nullptr;
  }

  for (aura::Window::Windows::const_reverse_iterator i =
           window->children().rbegin(); i != window->children().rend(); ++i) {
    gfx::NativeWindow result =
        GetLocalProcessWindowAtPointImpl(screen_point, ignore, *i);
    if (result)
      return result;
  }
  return NULL;
}

}  // namespace

gfx::NativeWindow GetLocalProcessWindowAtPointAsh(
    const gfx::Point& screen_point,
    const std::set<gfx::NativeWindow>& ignore) {
  return GetLocalProcessWindowAtPointImpl(
      screen_point, ignore, ash::wm::GetRootWindowAt(screen_point));
}

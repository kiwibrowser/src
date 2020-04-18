// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/window_finder_mus.h"

#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window.h"
#include "ui/views/mus/mus_client.h"

#if defined(USE_AURA)
#include "ui/aura/env.h"
#endif

namespace {

bool IsUsingMus() {
#if defined(USE_AURA)
  return aura::Env::GetInstance()->mode() == aura::Env::Mode::MUS;
#else
  return false;
#endif
}

}  // namespace

bool GetLocalProcessWindowAtPointMus(
    const gfx::Point& screen_point,
    const std::set<gfx::NativeWindow>& ignore,
    gfx::NativeWindow* mus_result) {
  *mus_result = nullptr;
  if (!IsUsingMus())
    return false;

  std::set<aura::Window*> root_windows =
      views::MusClient::Get()->window_tree_client()->GetRoots();
  // TODO(erg): Needs to deal with stacking order here.

  // For every mus window, look at the associated aura window and see if we're
  // in that.
  for (aura::Window* root : root_windows) {
    views::Widget* widget = views::Widget::GetWidgetForNativeView(root);
    if (widget && widget->GetWindowBoundsInScreen().Contains(screen_point)) {
      aura::Window* content_window = widget->GetNativeWindow();

      // If we were instructed to ignore this window, ignore it.
      if (base::ContainsKey(ignore, content_window))
        continue;

      *mus_result = content_window;
      return true;
    }
  }

  return true;
}

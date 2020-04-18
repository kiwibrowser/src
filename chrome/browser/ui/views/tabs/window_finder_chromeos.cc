// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/window_finder.h"
#include "chrome/browser/ui/views/tabs/window_finder_mus.h"

#include "ash/public/cpp/config.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "ui/aura/window.h"

gfx::NativeWindow GetLocalProcessWindowAtPointAsh(
    const gfx::Point& screen_point,
    const std::set<gfx::NativeWindow>& ignore);

gfx::NativeWindow WindowFinder::GetLocalProcessWindowAtPoint(
    const gfx::Point& screen_point,
    const std::set<gfx::NativeWindow>& ignore) {
  if (chromeos::GetAshConfig() == ash::Config::MASH) {
    gfx::NativeWindow mus_result = nullptr;
    if (GetLocalProcessWindowAtPointMus(screen_point, ignore, &mus_result))
      return mus_result;
  }

  return GetLocalProcessWindowAtPointAsh(screen_point, ignore);
}

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_WINDOW_FINDER_MUS_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_WINDOW_FINDER_MUS_H_

#include "chrome/browser/ui/views/tabs/window_finder.h"

// Used to locate the aura::Window under the specified point when in mus.
// If running in mus true is returned and |mus_result| is set to the
// aura::Window associated with the ui::Window under the specified point.
// It's possible for true to be returned and mus_result to be set to null.
bool GetLocalProcessWindowAtPointMus(
    const gfx::Point& screen_point,
    const std::set<gfx::NativeWindow>& ignore,
    gfx::NativeWindow* mus_result);

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_WINDOW_FINDER_MUS_H_

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/idle/idle.h"


#if defined(USE_X11)
#include "ui/base/idle/idle_query_x11.h"
#include "ui/base/idle/screensaver_window_finder_x11.h"
#endif

namespace ui {

void CalculateIdleTime(IdleTimeCallback notify) {
#if defined(USE_X11)
  IdleQueryX11 idle_query;
  notify.Run(idle_query.IdleTime());
#endif
}

bool CheckIdleStateIsLocked() {
#if defined(USE_X11)
  // Usually the screensaver is used to lock the screen.
  return ScreensaverWindowFinder::ScreensaverWindowExists();
#else
  return false;
#endif
}

}  // namespace ui

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_WINDOW_TRACKER_H_
#define UI_AURA_WINDOW_TRACKER_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/aura/window_observer.h"
#include "ui/base/window_tracker_template.h"

namespace aura {

using WindowTracker = ui::WindowTrackerTemplate<Window, WindowObserver>;

}  // namespace aura

#endif  // UI_AURA_WINDOW_TRACKER_H_

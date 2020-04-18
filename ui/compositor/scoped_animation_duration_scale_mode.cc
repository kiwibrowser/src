// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/scoped_animation_duration_scale_mode.h"

namespace ui {

// static
ScopedAnimationDurationScaleMode::DurationScaleMode
    ScopedAnimationDurationScaleMode::duration_scale_mode_ =
        ScopedAnimationDurationScaleMode::NORMAL_DURATION;

}  // namespace ui

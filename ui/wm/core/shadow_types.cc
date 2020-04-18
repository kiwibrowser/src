// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/core/shadow_types.h"

#include "ui/base/class_property.h"

namespace wm {

DEFINE_UI_CLASS_PROPERTY_KEY(int, kShadowElevationKey, kShadowElevationDefault);

void SetShadowElevation(aura::Window* window, int elevation) {
  window->SetProperty(kShadowElevationKey, elevation);
}

}  // namespace wm

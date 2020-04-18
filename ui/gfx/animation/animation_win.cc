// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/animation/animation.h"

#include <windows.h>

namespace gfx {

// static
bool Animation::ShouldRenderRichAnimationImpl() {
  BOOL result;
  // Get "Turn off all unnecessary animations" value.
  if (::SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &result, 0)) {
    return !!result;
  }
  return !::GetSystemMetrics(SM_REMOTESESSION);
}

// static
bool Animation::ScrollAnimationsEnabledBySystem() {
  return ShouldRenderRichAnimation();
}

} // namespace gfx

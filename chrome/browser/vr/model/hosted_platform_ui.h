// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_HOSTED_PLATFORM_UI_H_
#define CHROME_BROWSER_VR_MODEL_HOSTED_PLATFORM_UI_H_

#include "chrome/browser/vr/platform_ui_input_delegate.h"
#include "ui/gfx/geometry/rect_f.h"

namespace vr {
typedef PlatformUiInputDelegate* PlatformUiInputDelegatePtr;
struct HostedPlatformUi {
  bool hosted_ui_enabled = false;
  PlatformUiInputDelegatePtr delegate = nullptr;
  unsigned int texture_id = 0;
  bool floating = false;
  gfx::RectF rect;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_HOSTED_PLATFORM_UI_H_

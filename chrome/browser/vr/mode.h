// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODE_H_
#define CHROME_BROWSER_VR_MODE_H_

namespace vr {

// Specifies one of Chrome's VR modes.
// TODO(ymalik): These modes are currently only used for VR metrics. We should
// use model/ui_modes.h instead.
enum class Mode : int {
  kNoVr,
  kVr,          // All modes in VR.
  kVrBrowsing,  // Both kVrBrowsingRegular and kVrBrowsingFullscreen.
  kVrBrowsingRegular,
  kVrBrowsingFullscreen,  // Cinema mode.
  kWebXrVrPresentation,
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODE_H_

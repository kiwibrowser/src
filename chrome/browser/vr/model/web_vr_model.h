// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_WEB_VR_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_WEB_VR_MODEL_H_

namespace vr {

// As we wait for WebVR frames, we may pass through the following states.
enum WebVrState {
  // We are not awaiting a WebVR frame.
  kWebVrNoTimeoutPending = 0,
  // We are waiting for the minimum splash screen duration to be over. We're in
  // this state only during WebVR auto-presentation. During this phase, sending
  // VSync to the WebVR page is paused.
  kWebVrAwaitingMinSplashScreenDuration,
  kWebVrAwaitingFirstFrame,
  // We are awaiting a WebVR frame, and we will soon exceed the amount of time
  // that we're willing to wait. In this state, it could be appropriate to show
  // an affordance to the user to let them know that WebVR is delayed (eg, this
  // would be when we might show a spinner or progress bar).
  kWebVrTimeoutImminent,
  // In this case the time allotted for waiting for the first WebVR frame has
  // been entirely exceeded. This would, for example, be an appropriate time to
  // show "sad tab" UI to allow the user to bail on the WebVR content.
  kWebVrTimedOut,
  // We've received our first WebVR frame and are in WebVR presentation mode.
  kWebVrPresenting,
};

struct WebVrModel {
  WebVrState state = kWebVrNoTimeoutPending;
  bool has_received_permissions = false;
  bool showing_hosted_ui = false;
  bool presenting_web_vr() const {
    return state == kWebVrPresenting && !showing_hosted_ui;
  }
  bool awaiting_min_splash_screen_duration() const {
    return state == kWebVrAwaitingMinSplashScreenDuration;
  }
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_WEB_VR_MODEL_H_

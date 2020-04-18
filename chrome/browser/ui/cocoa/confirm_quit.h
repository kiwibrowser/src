// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONFIRM_QUIT_H_
#define CHROME_BROWSER_UI_COCOA_CONFIRM_QUIT_H_

class PrefRegistrySimple;

namespace confirm_quit {

enum ConfirmQuitMetric {
  // The user quit without having the feature enabled.
  kNoConfirm = 0,
  // The user held Cmd+Q for the entire duration.
  kHoldDuration,
  // The user hit Cmd+Q twice for the accelerated path.
  kDoubleTap,
  // The user tapped Cmd+Q once and then held it.
  kTapHold,

  kSampleCount
};

// Records the histogram value for the above metric.
void RecordHistogram(ConfirmQuitMetric sample);

// Registers the preference in app-wide local state.
void RegisterLocalState(PrefRegistrySimple* registry);

}  // namespace confirm_quit

#endif  // CHROME_BROWSER_UI_COCOA_CONFIRM_QUIT_H_

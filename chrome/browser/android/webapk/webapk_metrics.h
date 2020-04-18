// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_METRICS_H_
#define CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_METRICS_H_

namespace base {
class TimeDelta;
}

namespace webapk {

// Keep these enums up to date with tools/metrics/histograms/histograms.xml.
// Events for WebAPKs installation flow. The sum of InstallEvent histogram
// is the total number of times that a WebAPK infobar was triggered.
enum InstallEvent {
  // The user did not interact with the infobar.
  INFOBAR_IGNORED,
  // The infobar with the "Add-to-Homescreen" button is dismissed before the
  // installation started. "Dismiss" means the user closes the infobar by
  // clicking the "X" button.
  INFOBAR_DISMISSED_BEFORE_INSTALLATION,
  // The infobar with the "Adding" button is dismissed during installation.
  INFOBAR_DISMISSED_DURING_INSTALLATION,
  INSTALL_COMPLETED,
  INSTALL_FAILED,
  INSTALL_EVENT_MAX,
};

void TrackRequestTokenDuration(base::TimeDelta delta);
void TrackInstallDuration(base::TimeDelta delta);
void TrackInstallEvent(InstallEvent event);

};  // namespace webapk

#endif  // CHROME_BROWSER_ANDROID_WEBAPK_WEBAPK_METRICS_H_

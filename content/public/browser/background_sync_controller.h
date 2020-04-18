// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_BACKGROUND_SYNC_CONTROLLER_H_
#define CONTENT_PUBLIC_BROWSER_BACKGROUND_SYNC_CONTROLLER_H_

#include <stdint.h>

#include "content/common/content_export.h"
#include "url/gurl.h"

namespace content {

struct BackgroundSyncParameters;

// An interface that the Background Sync API uses to access services from the
// embedder. Must only be used on the UI thread.
class CONTENT_EXPORT BackgroundSyncController {
 public:
  virtual ~BackgroundSyncController() {}

  // This function allows the controller to alter the parameters used by
  // background sync. Note that disable can be overridden from false to true
  // but overrides from true to false will be ignored.
  virtual void GetParameterOverrides(
      BackgroundSyncParameters* parameters) const {};

  // Notification that a service worker registration with origin |origin| just
  // registered a background sync event.
  virtual void NotifyBackgroundSyncRegistered(const GURL& origin) {}

  // If |enabled|, ensures that the browser is running when the device next goes
  // online after |min_ms| has passed. The behavior is platform dependent:
  // * Android: Registers a GCM task which verifies that the browser is running
  // the next time the device goes online after |min_ms| has passed. If it's
  // not, it starts it.
  //
  // * Other Platforms: (UNIMPLEMENTED) Keeps the browser alive via
  // BackgroundModeManager until called with |enabled| = false. |min_ms| is
  // ignored.
  virtual void RunInBackground(bool enabled, int64_t min_ms) {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_BACKGROUND_SYNC_CONTROLLER_H_

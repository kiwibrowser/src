// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIDEO_CAPTURE_VIDEO_CAPTURE_SERVICE_EVENT_H_
#define SERVICES_VIDEO_CAPTURE_VIDEO_CAPTURE_SERVICE_EVENT_H_

#include "base/time/time.h"

namespace video_capture {
namespace uma {

// Used for logging capture events.
// Elements in this enum should not be deleted or rearranged; the only
// permitted operation is to add new elements before
// NUM_VIDEO_CAPTURE_SERVICE_EVENT.
enum VideoCaptureServiceEvent {
  BROWSER_USING_LEGACY_CAPTURE = 0,
  BROWSER_CONNECTING_TO_SERVICE = 1,
  SERVICE_STARTED = 2,
  SERVICE_SHUTTING_DOWN_BECAUSE_NO_CLIENT = 3,
  SERVICE_LOST_CONNECTION_TO_BROWSER = 4,
  BROWSER_LOST_CONNECTION_TO_SERVICE = 5,
  BROWSER_CLOSING_CONNECTION_TO_SERVICE = 6,  // No longer in use
  BROWSER_CLOSING_CONNECTION_TO_SERVICE_AFTER_ENUMERATION_ONLY = 7,
  BROWSER_CLOSING_CONNECTION_TO_SERVICE_AFTER_CAPTURE = 8,
  SERVICE_SHUTDOWN_TIMEOUT_CANCELED = 9,
  NUM_VIDEO_CAPTURE_SERVICE_EVENT
};

void LogVideoCaptureServiceEvent(VideoCaptureServiceEvent event);

void LogDurationFromLastConnectToClosingConnectionAfterEnumerationOnly(
    base::TimeDelta duration);
void LogDurationFromLastConnectToClosingConnectionAfterCapture(
    base::TimeDelta duration);
void LogDurationFromLastConnectToConnectionLost(base::TimeDelta duration);
void LogDurationUntilReconnectAfterEnumerationOnly(base::TimeDelta duration);
void LogDurationUntilReconnectAfterCapture(base::TimeDelta duration);

}  // namespace uma
}  // namespace video_capture

#endif  // SERVICES_VIDEO_CAPTURE_VIDEO_CAPTURE_SERVICE_EVENT_H_

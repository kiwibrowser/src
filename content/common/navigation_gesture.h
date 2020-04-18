// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_NAVIGATION_GESTURE_H_
#define CONTENT_COMMON_NAVIGATION_GESTURE_H_

namespace content {

enum NavigationGesture {
  // User initiated navigation/load.
  NavigationGestureUser,
  // Non-user initiated navigation/load. For example, onload- or
  // setTimeout-triggered document.location changes and form.submits.  See
  // http://b/1046841 for some cases that should be treated this way but aren't.
  NavigationGestureAuto,
  // Initial state.
  NavigationGestureUnknown,
  NavigationGestureLast = NavigationGestureUnknown
};

}  // namespace content

#endif  // CONTENT_COMMON_NAVIGATION_GESTURE_H_

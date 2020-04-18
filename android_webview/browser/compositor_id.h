// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_COMPOSITOR_ID_H_
#define ANDROID_WEBVIEW_BROWSER_COMPOSITOR_ID_H_

namespace android_webview {

struct CompositorID {
  CompositorID();
  CompositorID(int process_id, int routing_id);
  CompositorID(const CompositorID& other);
  CompositorID& operator=(const CompositorID& other);
  bool Equals(const CompositorID& other) const;

  int process_id;
  int routing_id;
};

struct CompositorIDComparator {
  bool operator()(const CompositorID& a, const CompositorID& b) const;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_COMPOSITOR_ID_H_

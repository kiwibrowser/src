// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/android/overscroll_refresh_handler.h"

#include "base/android/jni_android.h"
#include "jni/OverscrollRefreshHandler_jni.h"

using base::android::AttachCurrentThread;

namespace ui {

OverscrollRefreshHandler::OverscrollRefreshHandler(
    const base::android::JavaRef<jobject>& j_overscroll_refresh_handler) {
  j_overscroll_refresh_handler_.Reset(AttachCurrentThread(),
                                      j_overscroll_refresh_handler.obj());
}

OverscrollRefreshHandler::~OverscrollRefreshHandler() {}

bool OverscrollRefreshHandler::PullStart() {
  return Java_OverscrollRefreshHandler_start(AttachCurrentThread(),
                                             j_overscroll_refresh_handler_);
}

void OverscrollRefreshHandler::PullUpdate(float delta) {
  Java_OverscrollRefreshHandler_pull(AttachCurrentThread(),
                                     j_overscroll_refresh_handler_, delta);
}

void OverscrollRefreshHandler::PullRelease(bool allow_refresh) {
  Java_OverscrollRefreshHandler_release(
      AttachCurrentThread(), j_overscroll_refresh_handler_, allow_refresh);
}

void OverscrollRefreshHandler::PullReset() {
  Java_OverscrollRefreshHandler_reset(AttachCurrentThread(),
                                      j_overscroll_refresh_handler_);
}

}  // namespace ui

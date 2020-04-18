// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/overlay_window.h"

// static
std::unique_ptr<content::OverlayWindow> content::OverlayWindow::Create(
    PictureInPictureWindowController* controller) {
  return nullptr;
}

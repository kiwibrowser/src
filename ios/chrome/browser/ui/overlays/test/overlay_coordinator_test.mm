// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/overlays/test/overlay_coordinator_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

OverlayCoordinatorTest::OverlayCoordinatorTest() {
  queue_.SetBrowser(GetBrowser());
}

OverlayCoordinatorTest::~OverlayCoordinatorTest() {
  queue_.CancelOverlays();
}

void OverlayCoordinatorTest::StartOverlay() {
  queue_.AddOverlay(GetOverlay());
  queue_.StartNextOverlay();
}

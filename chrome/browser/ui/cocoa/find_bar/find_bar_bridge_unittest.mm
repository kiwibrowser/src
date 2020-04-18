// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/find_bar/find_bar_bridge.h"
#include "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/browser/ui/find_bar/find_bar_controller.h"

namespace {

class FindBarBridgeTest : public CocoaTest {
};

TEST_F(FindBarBridgeTest, Creation) {
  // Make sure the FindBarBridge constructor doesn't crash and
  // properly initializes its FindBarCocoaController.
  FindBarBridge bridge(NULL);
  EXPECT_TRUE(bridge.find_bar_cocoa_controller() != NULL);
}

TEST_F(FindBarBridgeTest, Accessors) {
  // Get/SetFindBarController are virtual methods implemented in
  // FindBarBridge, so we test them here.
  FindBarBridge* bridge = new FindBarBridge(NULL);
  FindBarController controller(bridge,  // takes ownership of |bridge|.
                               nullptr);
  bridge->SetFindBarController(&controller);

  EXPECT_EQ(&controller, bridge->GetFindBarController());
}
}  // namespace

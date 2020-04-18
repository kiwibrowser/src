// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_TEST_IOS_UI_VIEW_TEST_UTILS_H_
#define UI_BASE_TEST_IOS_UI_VIEW_TEST_UTILS_H_

#import <UIKit/UIKit.h>

namespace ui {
namespace test {
namespace uiview_utils {

// Forces rendering of a UIView. This is used in tests to make sure that UIKit
// optimizations don't have the views return the previous values (such as
// zoomScale).
void ForceViewRendering(UIView* view);

}  // namespace uiview_utils
}  // test
}  // namespace ui

#endif  // UI_BASE_TEST_IOS_UI_VIEW_TEST_UTILS_H_

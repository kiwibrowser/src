// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/test/ios/ui_view_test_utils.h"

#include "base/logging.h"

namespace ui {
namespace test {
namespace uiview_utils {

void ForceViewRendering(UIView* view) {
  DCHECK(view);
  CALayer* layer = view.layer;
  DCHECK(layer);
  // 19 is an arbitrary non-zero value.
  UIGraphicsBeginImageContext(CGSizeMake(19, 19));
  CGContext* context = UIGraphicsGetCurrentContext();
  DCHECK(context);
  [layer renderInContext:context];
  UIGraphicsEndImageContext();
}

}  // namespace uiview_utils
}  // namespace test
}  // namespace ui

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <UIKit/UIKit.h>

#include "ios/chrome/search_widget_extension/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ui_util {

CGFloat const kIconSpacing = 5;
CGFloat const kContentMargin = 12;

NSArray<NSLayoutConstraint*>* CreateSameConstraints(UIView* view1,
                                                    UIView* view2) {
  return @[
    [view1.leadingAnchor constraintEqualToAnchor:view2.leadingAnchor],
    [view1.trailingAnchor constraintEqualToAnchor:view2.trailingAnchor],
    [view1.topAnchor constraintEqualToAnchor:view2.topAnchor],
    [view1.bottomAnchor constraintEqualToAnchor:view2.bottomAnchor]
  ];
}

}  // namespace ui_util

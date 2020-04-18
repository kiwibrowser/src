// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_SEARCH_WIDGET_EXTENSION_UI_UTIL_H_
#define IOS_CHROME_SEARCH_WIDGET_EXTENSION_UI_UTIL_H_

namespace ui_util {

// The spacing to use between action icons.
extern CGFloat const kIconSpacing;

// The spacing between content and edges.
extern CGFloat const kContentMargin;

// Returns constraints to make two views' size and center equal by pinning
// leading, trailing, top and bottom anchors.
NSArray<NSLayoutConstraint*>* CreateSameConstraints(UIView* view1,
                                                    UIView* view2);

}  // namespace ui_util

#endif  // IOS_CHROME_SEARCH_WIDGET_EXTENSION_UI_UTIL_H_

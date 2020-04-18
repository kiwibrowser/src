// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_utils.h"

#include "base/feature_list.h"
#include "base/logging.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller_constants.h"
#import "ios/chrome/browser/ui/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

CGFloat ToolbarHeightWithTopOfScreenOffset(CGFloat status_bar_offset) {
  InterfaceIdiom idiom = IsIPadIdiom() ? IPAD_IDIOM : IPHONE_IDIOM;
  CGRect frame = kToolbarFrame[idiom];
  if (idiom == IPHONE_IDIOM) {
    frame.size.height += status_bar_offset;
  }
  return frame.size.height;
}

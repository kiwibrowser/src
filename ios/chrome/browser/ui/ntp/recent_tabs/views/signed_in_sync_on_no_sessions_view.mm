// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/recent_tabs/views/signed_in_sync_on_no_sessions_view.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/views_utils.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Desired height of the view.
const CGFloat kDesiredHeight = 130;

}  // anonymous namespace

@implementation SignedInSyncOnNoSessionsView

- (instancetype)initWithFrame:(CGRect)aRect {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    // Create and add the label.
    UILabel* noSessionLabel = recent_tabs::CreateMultilineLabel(
        l10n_util::GetNSString(IDS_IOS_OPEN_TABS_NO_SESSION_INSTRUCTIONS));
    [self addSubview:noSessionLabel];

    // Set constraints on the label.
    NSDictionary* viewsDictionary = @{ @"label" : noSessionLabel };
    NSArray* constraints =
        @[ @"V:|-16-[label]-(>=16)-|", @"H:|-16-[label]-16-|" ];
    ApplyVisualConstraints(constraints, viewsDictionary);
  }
  return self;
}

+ (CGFloat)desiredHeightInUITableViewCell {
  return kDesiredHeight;
}

@end

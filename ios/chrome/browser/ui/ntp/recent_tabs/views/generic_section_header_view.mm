// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/ntp/recent_tabs/views/generic_section_header_view.h"

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/browser/ui/ntp/recent_tabs/synced_sessions.h"
#include "ios/chrome/browser/ui/ntp/recent_tabs/views/disclosure_view.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/views_utils.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/l10n/time_format.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Desired height of the view.
const CGFloat kDesiredHeight = 48;

}  // namespace

@interface GenericSectionHeaderView () {
  DisclosureView* _disclosureView;
  UIImageView* _icon;
  UILabel* _label;
}
@end

@implementation GenericSectionHeaderView

- (instancetype)initWithFrame:(CGRect)aRect {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithType:(recent_tabs::SectionHeaderType)type
          sectionIsCollapsed:(BOOL)collapsed {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    _icon = [[UIImageView alloc] initWithImage:nil];
    [_icon setTranslatesAutoresizingMaskIntoConstraints:NO];
    _label = [[UILabel alloc] initWithFrame:CGRectZero];
    [_label setTranslatesAutoresizingMaskIntoConstraints:NO];
    [_label setFont:[[MDCTypography fontLoader] regularFontOfSize:16]];
    [_label setTextAlignment:NSTextAlignmentNatural];
    [_label setBackgroundColor:[UIColor whiteColor]];

    NSString* text = nil;
    NSString* imageName = nil;
    switch (type) {
      case recent_tabs::RECENTLY_CLOSED_TABS_SECTION_HEADER:
        text = l10n_util::GetNSString(IDS_IOS_RECENT_TABS_RECENTLY_CLOSED);
        imageName = @"ntp_recently_closed";
        break;
      case recent_tabs::OTHER_DEVICES_SECTION_HEADER:
        text = l10n_util::GetNSString(IDS_IOS_RECENT_TABS_OTHER_DEVICES);
        imageName = @"ntp_opentabs_laptop";
        break;
    }
    DCHECK(text);
    DCHECK(imageName);
    [_label setText:text];
    [_icon setImage:
               [[UIImage imageNamed:imageName]
                   imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate]];

    self.isAccessibilityElement = YES;
    self.accessibilityLabel = [_label accessibilityLabel];
    self.accessibilityTraits |=
        UIAccessibilityTraitButton | UIAccessibilityTraitHeader;

    _disclosureView = [[DisclosureView alloc] init];
    [_disclosureView setTranslatesAutoresizingMaskIntoConstraints:NO];

    [self addSubview:_icon];
    [self addSubview:_label];
    [self addSubview:_disclosureView];

    NSDictionary* viewsDictionary = @{
      @"icon" : _icon,
      @"label" : _label,
      @"disclosureView" : _disclosureView,
    };
    NSArray* constraints = @[
      @"H:|-16-[icon]-16-[label]-(>=16)-[disclosureView]-16-|",
      @"V:|-12-[label]-12-|"
    ];
    ApplyVisualConstraints(constraints, viewsDictionary);
    [self addConstraint:[NSLayoutConstraint
                            constraintWithItem:_disclosureView
                                     attribute:NSLayoutAttributeCenterY
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:self
                                     attribute:NSLayoutAttributeCenterY
                                    multiplier:1.0
                                      constant:0]];
    [self addConstraint:[NSLayoutConstraint
                            constraintWithItem:_icon
                                     attribute:NSLayoutAttributeCenterY
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:_label
                                     attribute:NSLayoutAttributeCenterY
                                    multiplier:1.0
                                      constant:0]];
    [self setSectionIsCollapsed:collapsed animated:NO];
  }
  return self;
}

+ (CGFloat)desiredHeightInUITableViewCell {
  return kDesiredHeight;
}

#pragma mark - HeaderOfCollapsableSectionProtocol

- (void)setSectionIsCollapsed:(BOOL)collapsed animated:(BOOL)animated {
  [_disclosureView setTransformWhenCollapsed:collapsed animated:animated];
  UIColor* tintColor = (collapsed ? recent_tabs::GetIconColorGray()
                                  : recent_tabs::GetIconColorBlue());
  [self setTintColor:tintColor];
  UIColor* textColor = (collapsed ? recent_tabs::GetTextColorGray()
                                  : recent_tabs::GetTextColorBlue());
  [_label setTextColor:textColor];
  self.accessibilityHint =
      collapsed ? l10n_util::GetNSString(
                      IDS_IOS_RECENT_TABS_DISCLOSURE_VIEW_COLLAPSED_HINT)
                : l10n_util::GetNSString(
                      IDS_IOS_RECENT_TABS_DISCLOSURE_VIEW_EXPANDED_HINT);
}

@end

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/ntp/recent_tabs/views/session_tab_data_view.h"

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "components/grit/components_scaled_resources.h"
#import "ios/chrome/browser/favicon/favicon_loader.h"
#include "ios/chrome/browser/favicon/ios_chrome_favicon_loader_factory.h"
#include "ios/chrome/browser/ui/ntp/recent_tabs/synced_sessions.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/views_utils.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_utils.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/resource/resource_bundle.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Desired height of the view.
const CGFloat kDesiredHeight = 48;

}  // namespace

@interface SessionTabDataView () {
  UIImageView* _favicon;
  UILabel* _label;
}
@end

@implementation SessionTabDataView

- (instancetype)initWithFrame:(CGRect)aRect {
  self = [super initWithFrame:aRect];
  if (self) {
    _favicon = [[UIImageView alloc] initWithImage:nil];
    [_favicon setTranslatesAutoresizingMaskIntoConstraints:NO];

    _label = [[UILabel alloc] initWithFrame:CGRectZero];
    [_label setLineBreakMode:NSLineBreakByTruncatingTail];
    [_label setTranslatesAutoresizingMaskIntoConstraints:NO];
    [_label setFont:[[MDCTypography fontLoader] regularFontOfSize:16]];
    [_label setTextAlignment:NSTextAlignmentNatural];
    [_label setTextColor:recent_tabs::GetTextColorGray()];
    [_label setHighlightedTextColor:[_label textColor]];
    [_label setBackgroundColor:[UIColor whiteColor]];

    self.isAccessibilityElement = YES;
    self.accessibilityTraits |= UIAccessibilityTraitButton;

    [self addSubview:_favicon];
    [self addSubview:_label];

    NSDictionary* viewsDictionary = @{
      @"favicon" : _favicon,
      @"label" : _label,
    };

    NSArray* constraints = @[
      @"H:|-56-[favicon(==16)]-16-[label]-16-|",
      @"V:|-(>=0)-[favicon(==16)]-(>=0)-|"
    ];
    ApplyVisualConstraints(constraints, viewsDictionary);

    [self addConstraint:[NSLayoutConstraint
                            constraintWithItem:_favicon
                                     attribute:NSLayoutAttributeCenterY
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:self
                                     attribute:NSLayoutAttributeCenterY
                                    multiplier:1.0
                                      constant:0]];
    [self addConstraint:[NSLayoutConstraint
                            constraintWithItem:_label
                                     attribute:NSLayoutAttributeCenterY
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:self
                                     attribute:NSLayoutAttributeCenterY
                                    multiplier:1.0
                                      constant:0]];
  }
  return self;
}

- (void)updateWithDistantTab:(synced_sessions::DistantTab const*)distantTab
                browserState:(ios::ChromeBrowserState*)browserState {
  DCHECK(distantTab);
  DCHECK(browserState);
  NSString* text = base::SysUTF16ToNSString(distantTab->title);
  GURL url = distantTab->virtual_url;
  [self setText:text url:url browserState:browserState];
}

- (void)setText:(NSString*)text
             url:(const GURL&)url
    browserState:(ios::ChromeBrowserState*)browserState {
  DCHECK(text);
  DCHECK(browserState);
  [_label setText:text];
  self.accessibilityLabel = [_label accessibilityLabel];
  TabSwitcherGetFavicon(url, browserState, ^(UIImage* newIcon) {
    [_favicon setImage:newIcon];
  });
}

- (void)updateWithTabRestoreEntry:
            (const sessions::TabRestoreService::Entry*)entry
                     browserState:(ios::ChromeBrowserState*)browserState {
  DCHECK(entry);
  DCHECK(browserState);
  switch (entry->type) {
    case sessions::TabRestoreService::TAB: {
      const sessions::TabRestoreService::Tab* tab =
          static_cast<const sessions::TabRestoreService::Tab*>(entry);
      const sessions::SerializedNavigationEntry& entry =
          tab->navigations[tab->current_navigation_index];
      // Use the page's title for the label, or its URL if title is empty.
      NSString* text;
      if (entry.title().size()) {
        text = base::SysUTF16ToNSString(entry.title());
      } else {
        text = base::SysUTF8ToNSString(entry.virtual_url().spec());
      }
      [self setText:text url:entry.virtual_url() browserState:browserState];
      break;
    }
    case sessions::TabRestoreService::WINDOW: {
      // We only handle the TAB type.
      [_label setText:@"Window type - NOTIMPLEMENTED"];
      [_favicon setImage:[UIImage imageNamed:@"tools_bookmark"]];
      break;
    }
  }
}

+ (CGFloat)desiredHeightInUITableViewCell {
  return kDesiredHeight;
}

@end

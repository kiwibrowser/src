// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/tab_favicon_view.h"

#include "base/mac/scoped_cftyperef.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/grit/theme_resources.h"
#include "components/grit/components_scaled_resources.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/geometry/angle_conversions.h"
#include "ui/native_theme/native_theme.h"

namespace {
constexpr CGFloat kSadTabAnimationTime = 0.5;
}  // namespace

@implementation TabFaviconView {
  TabSpinnerView* spinnerView_;  // Weak.
  NSImageView* iconView_;        // Weak.
}

@synthesize tabLoadingState = tabLoadingState_;

- (instancetype)initWithFrame:(NSRect)frame {
  if (self = [super initWithFrame:frame]) {
    NSRect subviewFrame = NSMakeRect(0, 0, frame.size.width, frame.size.height);
    spinnerView_ =
        [[[TabSpinnerView alloc] initWithFrame:subviewFrame] autorelease];
    [self addSubview:spinnerView_];
    iconView_ = [[[NSImageView alloc] initWithFrame:subviewFrame] autorelease];
    [self addSubview:iconView_];
  }

  return self;
}

- (NSImage*)sadTabIcon {
  NSImage* sadTabIcon = ui::ResourceBundle::GetSharedInstance()
                            .GetNativeImageNamed(IDR_CRASH_SAD_FAVICON)
                            .AsNSImage();
  BOOL hasDarkTheme =
      [[self window] respondsToSelector:@selector(hasDarkTheme)] &&
      [[self window] hasDarkTheme];

  if (hasDarkTheme) {
    NSRect bounds = NSZeroRect;
    bounds.size = [sadTabIcon size];

    return [NSImage imageWithSize:bounds.size
                          flipped:NO
                   drawingHandler:^BOOL(NSRect destRect) {
                     [[NSColor whiteColor] set];
                     NSRectFill(destRect);
                     [sadTabIcon drawInRect:destRect
                                   fromRect:NSZeroRect
                                  operation:NSCompositeDestinationIn
                                   fraction:1.0];
                     return YES;
                   }];
  }

  return sadTabIcon;
}

- (void)setTabLoadingState:(TabLoadingState)newLoadingState
                   forIcon:(NSImage*)anImage {
  // Always update the tab done icon, otherwise exit if the state has not
  // changed.
  if (newLoadingState != kTabDone && tabLoadingState_ == newLoadingState) {
    return;
  }

  tabLoadingState_ = newLoadingState;

  BOOL showingAnIcon =
      (tabLoadingState_ == kTabCrashed || tabLoadingState_ == kTabDone);

  [spinnerView_ setHidden:showingAnIcon];
  [iconView_ setHidden:!showingAnIcon];

  if (tabLoadingState_ == kTabCrashed) {
    CAMediaTimingFunction* linearTimingFunction =
        [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear];
    NSImage* sadTabIcon = [self sadTabIcon];

    [NSAnimationContext beginGrouping];

    [[NSAnimationContext currentContext] setDuration:kSadTabAnimationTime];
    [[NSAnimationContext currentContext]
        setTimingFunction:linearTimingFunction];
    [[NSAnimationContext currentContext] setCompletionHandler:^{
      [NSAnimationContext beginGrouping];

      [iconView_ setImage:sadTabIcon];
      [[NSAnimationContext currentContext] setDuration:kSadTabAnimationTime];
      [[NSAnimationContext currentContext]
          setTimingFunction:linearTimingFunction];
      [[iconView_ animator] setFrameOrigin:NSZeroPoint];

      [NSAnimationContext endGrouping];
    }];

    [[iconView_ animator]
        setFrameOrigin:NSMakePoint(0, -NSHeight([self bounds]))];

    [NSAnimationContext endGrouping];
  } else if (tabLoadingState_ == kTabDone) {
    [iconView_ setImage:anImage];
  } else if (tabLoadingState_ == kTabWaiting) {
    [spinnerView_ setSpinDirection:SpinDirection::REVERSE];
  } else {
    [spinnerView_ setSpinDirection:SpinDirection::FORWARD];
  }
}

- (void)setTabLoadingState:(TabLoadingState)newLoadingState {
  [self setTabLoadingState:newLoadingState forIcon:nil];
}

- (void)setTabDoneStateWithIcon:(NSImage*)anImage {
  [self setTabLoadingState:kTabDone forIcon:anImage];
}

// ThemedWindowDrawing implementation.

- (void)windowDidChangeTheme {
  // If showing the sad tab icon, make sure that icon matches the current theme.
  if (tabLoadingState_ == kTabCrashed) {
    [iconView_ setImage:[self sadTabIcon]];
  }
}

- (void)windowDidChangeActive {
}

@end

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/infobars/infobar_background_view.h"

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/themes/theme_properties.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/theme_provider.h"

@implementation InfoBarBackgroundView

- (id)initWithFrame:(NSRect)frame {
  self = [super initWithFrame:frame];
  return self;
}

- (id)initWithCoder:(NSCoder*)decoder {
  self = [super initWithCoder:decoder];
  return self;
}

- (NSColor*)strokeColor {
  const ui::ThemeProvider* themeProvider = [[self window] themeProvider];
  if (!themeProvider)
    return [NSColor blackColor];

  BOOL active = [[self window] isMainWindow];
  return themeProvider->GetNSColor(
      active ? ThemeProperties::COLOR_TOOLBAR_STROKE
             : ThemeProperties::COLOR_TOOLBAR_STROKE_INACTIVE);
}

- (void)drawRect:(NSRect)rect {
  NSRect bounds = [self bounds];

  // Draw the background.
  // TODO(ellyjones): Use the detached bookmark bar color here.
  [[NSColor whiteColor] set];
  NSRectFill([self bounds]);

  NSColor* strokeColor = [self strokeColor];
  if (strokeColor) {
    [strokeColor set];

    // Stroke the bottom of the infobar.
    NSRect borderRect, contentRect;
    NSDivideRect(bounds, &borderRect, &contentRect, 1, NSMinYEdge);
    NSRectFillUsingOperation(borderRect, NSCompositeSourceOver);
  }
}

- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

// This view is intentionally not opaque because it overlaps with the findbar.

- (BOOL)accessibilityIsIgnored {
  return NO;
}

- (id)accessibilityAttributeValue:(NSString*)attribute {
  if ([attribute isEqual:NSAccessibilityRoleAttribute])
    return NSAccessibilityGroupRole;

  return [super accessibilityAttributeValue:attribute];
}

@end

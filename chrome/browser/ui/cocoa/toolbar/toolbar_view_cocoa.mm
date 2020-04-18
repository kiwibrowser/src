// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/toolbar/toolbar_view_cocoa.h"

#import "chrome/browser/ui/cocoa/view_id_util.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/material_design/material_design_controller.h"

@implementation ToolbarView

@synthesize dividerOpacity = dividerOpacity_;

// Prevent mouse down events from moving the parent window around.
- (BOOL)mouseDownCanMoveWindow {
  return NO;
}

- (void)drawRect:(NSRect)dirtyRect {
  [self drawBackground:dirtyRect];
}

- (BOOL)accessibilityIsIgnored {
  return NO;
}

- (id)accessibilityAttributeValue:(NSString*)attribute {
  if ([attribute isEqual:NSAccessibilityRoleAttribute])
    return NSAccessibilityToolbarRole;

  return [super accessibilityAttributeValue:attribute];
}

- (ViewID)viewID {
  return VIEW_ID_TOOLBAR;
}

- (BOOL)isOpaque {
  return YES;
}

// ThemedWindowDrawing overrides.

- (void)windowDidChangeActive {
  // Need to redraw the omnibox and toolbar buttons as well.
  [self cr_recursivelySetNeedsDisplay:YES];
}

@end

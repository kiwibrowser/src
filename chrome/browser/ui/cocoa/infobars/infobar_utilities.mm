// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/infobars/infobar_utilities.h"

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_container_controller.h"
#import "components/infobars/core/infobar.h"
#import "ui/base/cocoa/nsview_additions.h"

@interface InfobarLabelTextField : NSTextField
@end

@implementation InfobarLabelTextField

- (void)drawRect:(NSRect)rect {
  NSView* infobarBackgroundView = [self superview];
  [self cr_drawUsingAncestor:infobarBackgroundView inRect:rect];
  [super drawRect:rect];
}

- (BOOL)isOpaque {
  return YES;
}

@end

namespace InfoBarUtilities {

// Move the |toMove| view |spacing| pixels before/after the |anchor| view.
// |after| signifies the side of |anchor| on which to place |toMove|.
void MoveControl(NSView* anchor, NSView* toMove, int spacing, bool after) {
  NSRect anchorFrame = [anchor frame];
  NSRect toMoveFrame = [toMove frame];

  // At the time of this writing, OS X doesn't natively support BiDi UIs, but
  // it doesn't hurt to be forward looking.
  bool toRight = after;

  if (toRight) {
    toMoveFrame.origin.x = NSMaxX(anchorFrame) + spacing;
  } else {
    // Place toMove to theleft of anchor.
    toMoveFrame.origin.x = NSMinX(anchorFrame) -
        spacing - NSWidth(toMoveFrame);
  }
  [toMove setFrame:toMoveFrame];
}

// Creates a label control in the style we need for the infobar's labels
// within |bounds|.
NSTextField* CreateLabel(NSRect bounds) {
  NSTextField* ret = [[InfobarLabelTextField alloc] initWithFrame:bounds];
  [ret setEditable:NO];
  [ret setDrawsBackground:NO];
  [ret setBordered:NO];
  return ret;
}

// Adds an item with the specified properties to |menu|.
void AddMenuItem(NSMenu* menu,
                 id target,
                 SEL selector,
                 NSString* title,
                 int tag,
                 bool enabled,
                 bool checked,
                 NSString* representedObject) {
  if (tag == -1) {
    [menu addItem:[NSMenuItem separatorItem]];
  } else {
    base::scoped_nsobject<NSMenuItem> item(
        [[NSMenuItem alloc] initWithTitle:title
                                   action:selector
                            keyEquivalent:@""]);
    [item setTag:tag];
    [menu addItem:item];
    [item setTarget:target];
    if (representedObject != nil) {
      [item setRepresentedObject:representedObject];
    }
    if (checked)
      [item setState:NSOnState];
    if (!enabled)
      [item setEnabled:NO];
  }
}

}  // namespace InfoBarUtilities

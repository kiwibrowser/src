// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/menu_button.h"

#include "base/logging.h"
#import "chrome/browser/ui/cocoa/clickhold_button_cell.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "ui/base/material_design/material_design_controller.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/material_design/material_design_controller.h"

@interface MenuButton (Private)
- (void)showMenu:(BOOL)isDragging;
- (void)clickShowMenu:(id)sender;
- (void)dragShowMenu:(id)sender;
@end  // @interface MenuButton (Private)

@implementation MenuButton

@synthesize openMenuOnClick = openMenuOnClick_;
@synthesize openMenuOnRightClick = openMenuOnRightClick_;
@synthesize openMenuOnClickHold = openMenuOnClickHold_;

// Overrides:

+ (Class)cellClass {
  return [ClickHoldButtonCell class];
}

- (id)init {
  if ((self = [super init])) {
    openMenuOnClickHold_ = YES;
    [self configureCell];
  }
  return self;
}

- (id)initWithCoder:(NSCoder*)decoder {
  if ((self = [super initWithCoder:decoder])) {
    openMenuOnClickHold_ = YES;
    [self configureCell];
  }
  return self;
}

- (id)initWithFrame:(NSRect)frameRect {
  if ((self = [super initWithFrame:frameRect])) {
    openMenuOnClickHold_ = YES;
    [self configureCell];
  }
  return self;
}

- (void)dealloc {
  self.attachedMenu = nil;
  [super dealloc];
}

- (void)awakeFromNib {
  [self configureCell];
}

- (void)setCell:(NSCell*)cell {
  [super setCell:cell];
  [self configureCell];
}

- (void)rightMouseDown:(NSEvent*)theEvent {
  if (!openMenuOnRightClick_) {
    [super rightMouseDown:theEvent];
    return;
  }

  [self clickShowMenu:self];
}

// Accessors and mutators:

- (NSMenu*)attachedMenu {
  return attachedMenu_.get();
}

- (void)setAttachedMenu:(NSMenu*)menu {
  attachedMenu_.reset([menu retain]);
  [self configureCell];
}

- (void)setOpenMenuOnClick:(BOOL)enabled {
  openMenuOnClick_ = enabled;
  [self configureCell];
}

- (void)setOpenMenuOnRightClick:(BOOL)enabled {
  openMenuOnRightClick_ = enabled;
  [self configureCell];
}

- (void)setOpenMenuOnClickHold:(BOOL)enabled {
  openMenuOnClickHold_ = enabled;
  [self configureCell];
}

- (NSRect)menuRect {
  return [self bounds];
}

@end  // @implementation MenuButton

@implementation MenuButton (Private)

// Synchronize the state of this class with its ClickHoldButtonCell.
- (void)configureCell {
  ClickHoldButtonCell* cell = [self cell];
  DCHECK([cell isKindOfClass:[ClickHoldButtonCell class]]);

  if (![self attachedMenu]) {
    [cell setEnableClickHold:NO];
    [cell setEnableRightClick:NO];
    [cell setClickHoldAction:nil];
    [cell setClickHoldTarget:nil];
    [cell setAccessibilityShowMenuAction:nil];
    [cell setAccessibilityShowMenuTarget:nil];
    return;
  }

  if (openMenuOnClick_) {
    [cell setClickHoldTimeout:0.0];  // Make menu trigger immediately.
    [cell setAction:@selector(clickShowMenu:)];
    [cell setTarget:self];
  } else {
    [cell setClickHoldTimeout:0.25];  // Default value.
  }

  // Regardless of the left-or-right click behaviors, the button needs to allow
  // the classic click-hold-drag behavior to "pull open" the menu.
  [cell setEnableClickHold:openMenuOnClickHold_];
  [cell setClickHoldAction:@selector(dragShowMenu:)];
  [cell setClickHoldTarget:self];

  [cell setEnableRightClick:openMenuOnRightClick_];
  if (!openMenuOnClick_ || openMenuOnRightClick_) {
    [cell setAccessibilityShowMenuAction:@selector(clickShowMenu:)];
    [cell setAccessibilityShowMenuTarget:self];
  } else {
    [cell setAccessibilityShowMenuAction:nil];
    [cell setAccessibilityShowMenuTarget:nil];
  }
}

// Actually show the menu (in the correct location). |isDragging| indicates
// whether the mouse button is still down or not.
- (void)showMenu:(BOOL)isDragging {
  if (![self attachedMenu]) {
    LOG(WARNING) << "No menu available.";
    if (isDragging) {
      // If we're dragging, wait for mouse up.
      [NSApp nextEventMatchingMask:NSLeftMouseUpMask
                         untilDate:[NSDate distantFuture]
                            inMode:NSEventTrackingRunLoopMode
                           dequeue:YES];
    }
    return;
  }

  // TODO(viettrungluu): We have some fudge factors below to make things line up
  // (approximately). I wish I knew how to get rid of them. (Note that our view
  // is flipped, and that frame should be in our coordinates.) The y/height is
  // very odd, since it doesn't seem to respond to changes the way that it
  // should. I don't understand it.
  NSRect frame = [self menuRect];
  frame.origin.x -= 2.0;
  frame.size.height -= 19.0 - NSHeight(frame);

  // The button's icon has a different relationship to its bounds, so have to
  // adjust the menu location by that delta.
  frame.origin.x -= [ToolbarController materialDesignButtonInset];
  frame.origin.y += [ToolbarController materialDesignButtonInset];

  // Make our pop-up button cell and set things up. This is, as of 10.5, the
  // official Apple-recommended hack. Later, perhaps |-[NSMenu
  // popUpMenuPositioningItem:atLocation:inView:]| may be a better option.
  // However, using a pulldown has the benefit that Cocoa automatically places
  // the menu correctly even when we're at the edge of the screen (including
  // "dragging upwards" when the button is close to the bottom of the screen).
  // A |scoped_nsobject| local variable cannot be used here because
  // Accessibility on 10.5 grabs the NSPopUpButtonCell without retaining it, and
  // uses it later. (This is fixed in 10.6.)
  if (!popUpCell_.get()) {
    popUpCell_.reset([[NSPopUpButtonCell alloc] initTextCell:@""
                                                   pullsDown:YES]);
  }
  DCHECK(popUpCell_.get());
  [popUpCell_ setMenu:[self attachedMenu]];
  [popUpCell_ selectItem:nil];
  [popUpCell_ attachPopUpWithFrame:frame inView:self];
  [popUpCell_ performClickWithFrame:frame inView:self];

  // Once the menu is dismissed send a mouseExited event if necessary. If the
  // menu action caused the super view to resize then we won't automatically
  // get a mouseExited event so we need to do this manually.
  // See http://crbug.com/82456
  if (![self cr_isMouseInView]) {
    if ([[self cell] respondsToSelector:@selector(mouseExited:)])
      [[self cell] mouseExited:nil];
  }
}

// Called when the button is clicked and released. (Shouldn't happen with
// timeout of 0, though there may be some strange pointing devices out there.)
- (void)clickShowMenu:(id)sender {
  // We shouldn't get here unless the menu is enabled.
  DCHECK([self attachedMenu]);
  [self showMenu:NO];
}

// Called when the button is clicked and dragged/held.
- (void)dragShowMenu:(id)sender {
  // We shouldn't get here unless the menu is enabled.
  DCHECK([self attachedMenu]);
  [self showMenu:YES];
}

@end  // @implementation MenuButton (Private)

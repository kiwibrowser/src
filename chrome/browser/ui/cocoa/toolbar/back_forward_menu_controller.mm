// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/toolbar/back_forward_menu_controller.h"

#include <memory>

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/menu_button.h"
#include "chrome/browser/ui/toolbar/back_forward_menu_model.h"
#import "ui/events/event_utils.h"
#include "ui/gfx/image/image.h"

using base::SysUTF16ToNSString;

@implementation BackForwardMenuController

// Accessors and mutators:

@synthesize type = type_;

// Own methods:

- (id)initWithBrowser:(Browser*)browser
            modelType:(BackForwardMenuType)type
               button:(MenuButton*)button {
  if ((self = [super init])) {
    type_ = type;
    button_ = button;
    model_.reset(new BackForwardMenuModel(browser, type_));
    DCHECK(model_.get());
    backForwardMenu_.reset([[NSMenu alloc] initWithTitle:@""]);
    DCHECK(backForwardMenu_.get());
    [backForwardMenu_ setDelegate:self];

    [button_ setAttachedMenu:backForwardMenu_];
    [button_ setOpenMenuOnClick:NO];
  }
  return self;
}

- (void)browserWillBeDestroyed {
  [button_ setAttachedMenu:nil];
  [backForwardMenu_ setDelegate:nil];
  backForwardMenu_.reset();
  model_.reset();
}

// Methods as delegate:

// Called by backForwardMenu_ just before tracking begins.
//TODO(viettrungluu): should we do anything for chapter stops (see model)?
- (void)menuNeedsUpdate:(NSMenu*)menu {
  DCHECK(menu == backForwardMenu_);

  // Remove old menu items (backwards order is as good as any).
  for (NSInteger i = [menu numberOfItems]; i > 0; i--)
    [menu removeItemAtIndex:(i - 1)];

  // 0-th item must be blank. (This is because we use a pulldown list, for which
  // Cocoa uses the 0-th item as "title" in the button.)
  [menu insertItemWithTitle:@""
                     action:nil
              keyEquivalent:@""
                    atIndex:0];
  for (int menuID = 0; menuID < model_->GetItemCount(); menuID++) {
    if (model_->IsSeparator(menuID)) {
      [menu insertItem:[NSMenuItem separatorItem]
               atIndex:(menuID + 1)];
    } else {
      // Create a menu item with the right label.
      NSMenuItem* menuItem = [[NSMenuItem alloc]
              initWithTitle:SysUTF16ToNSString(model_->GetLabelAt(menuID))
                     action:nil
              keyEquivalent:@""];
      [menuItem autorelease];

      gfx::Image icon;
      // Icon (if it has one).
      if (model_->GetIconAt(menuID, &icon))
        [menuItem setImage:icon.ToNSImage()];

      // This will make it call our |-executeMenuItem:| method. We store the
      // |menuID| (or |menu_id|) in the tag.
      [menuItem setTag:menuID];
      [menuItem setTarget:self];
      [menuItem setAction:@selector(executeMenuItem:)];

      // Put it in the menu!
      [menu insertItem:menuItem
               atIndex:(menuID + 1)];
    }
  }
}

// Action methods:

- (void)executeMenuItem:(id)sender {
  DCHECK([sender isKindOfClass:[NSMenuItem class]]);
  int menuID = [sender tag];
  int event_flags = ui::EventFlagsFromNative([NSApp currentEvent]);
  model_->ActivatedAt(menuID, event_flags);
}

@end  // @implementation BackForwardMenuController

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/new_tab_menu_view_item.h"

#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/commands/open_new_tab_command.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface NewTabMenuViewItem ()
@property(nonatomic, readonly, getter=isIncognito) BOOL incognito;
@end

@implementation NewTabMenuViewItem

- (BOOL)isIncognito {
  return NO;
}

- (BOOL)canExecuteCommand {
  return YES;
}

- (void)executeCommandWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher {
  UIView* view = self.tableViewCell;
  CGPoint center = [view.superview convertPoint:view.center toView:view.window];
  OpenNewTabCommand* command =
      [[OpenNewTabCommand alloc] initWithIncognito:self.isIncognito
                                       originPoint:center];
  [dispatcher openNewTab:command];
}

@end

@implementation NewIncognitoTabMenuViewItem

- (BOOL)isIncognito {
  return YES;
}

@end

// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_context_menu_cocoa_controller.h"

#include <vector>

#include "base/macros.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/bookmarks/bookmark_context_menu_controller.h"
#include "chrome/browser/ui/browser.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#include "components/bookmarks/browser/bookmark_model.h"
#import "ui/base/cocoa/menu_controller.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

@interface BookmarkContextMenuCocoaController (Private)
- (void)willExecuteCommand:(int)command;
@end

class BookmarkContextMenuDelegateBridge :
    public BookmarkContextMenuControllerDelegate {
 public:
  BookmarkContextMenuDelegateBridge(
      BookmarkContextMenuCocoaController* controller)
      : controller_(controller) {
  }

  ~BookmarkContextMenuDelegateBridge() override {}

  void CloseMenu() override { [controller_ cancelTracking]; }

  void WillExecuteCommand(
      int command_id,
      const std::vector<const BookmarkNode*>& bookmarks) override {
    [controller_ willExecuteCommand:command_id];
  }

 private:
  BookmarkContextMenuCocoaController* controller_;  // Weak; owns us.

  DISALLOW_COPY_AND_ASSIGN(BookmarkContextMenuDelegateBridge);
};

@implementation BookmarkContextMenuCocoaController

- (id)initWithBookmarkBarController:(BookmarkBarController*)controller {
  if ((self = [super init])) {
    bookmarkBarController_ = controller;
    bridge_.reset(new BookmarkContextMenuDelegateBridge(self));
  }
  return self;
}

// Re-creates |bookmarkContextMenuController_| and |menuController_| based on
// |bookmarkModel| and the current value of |bookmarkNode_|.
- (void)createMenuControllers:(BookmarkModel*)bookmarkModel {
  const BookmarkNode* parent = NULL;
  std::vector<const BookmarkNode*> nodes;
  if (bookmarkNode_ == bookmarkModel->other_node()) {
    nodes.push_back(bookmarkNode_);
    parent = bookmarkModel->bookmark_bar_node();
  } else if (bookmarkNode_ == bookmarkModel->bookmark_bar_node()) {
    parent = bookmarkModel->bookmark_bar_node();
    nodes.push_back(parent);
  } else if (bookmarkNode_) {
    nodes.push_back(bookmarkNode_);
    parent = bookmarkNode_->parent();
  }

  // Close old menu if it's open.
  // crbug.com/749213
  [menuController_ cancel];
  Browser* browser = [bookmarkBarController_ browser];
  bookmarkContextMenuController_.reset(
      new BookmarkContextMenuController([bookmarkBarController_ browserWindow],
                                        bridge_.get(), browser,
                                        browser->profile(), browser, parent,
                                        nodes));
  ui::SimpleMenuModel* menuModel =
      bookmarkContextMenuController_->menu_model();
  menuController_.reset([[MenuControllerCocoa alloc] initWithModel:menuModel
                                            useWithPopUpButtonCell:NO]);
}

- (BookmarkModel*)bookmarkModel {
  // Depending on timing, the model may not yet have been loaded.
  BookmarkModel* bookmarkModel = [bookmarkBarController_ bookmarkModel];
  if (!bookmarkModel || !bookmarkModel->loaded())
    return nil;
  return bookmarkModel;
}

- (NSMenu*)menuForBookmarkNode:(const BookmarkNode*)node {
  BookmarkModel* bookmarkModel = [self bookmarkModel];

  // This may be called before the BMB view has been added to the window. In
  // that case, simply return nil so that BookmarkContextMenuController doesn't
  // get created with a nil window.
  if (!bookmarkModel || ![bookmarkBarController_ browserWindow])
    return nil;

  // Rebuild the menu if it's for a different node than the last request.
  if (!menuController_ || node != bookmarkNode_) {
    bookmarkNode_ = node;
    [self createMenuControllers:bookmarkModel];
  }
  return [menuController_ menu];
}

- (NSMenu*)menuForBookmarkBar {
  BookmarkModel* bookmarkModel = [self bookmarkModel];
  if (!bookmarkModel)
    return nil;

  return [self menuForBookmarkNode:bookmarkModel->bookmark_bar_node()];
}

- (void)willExecuteCommand:(int)command {
  // Some items should not close currently-open sub-folder menus.
  switch (command) {
    case IDC_CUT:
    case IDC_COPY:
    case IDC_PASTE:
    case IDC_BOOKMARK_BAR_REMOVE:
      return;
  }

  [bookmarkBarController_ closeFolderAndStopTrackingMenus];
  if (bookmarkNode_)
    [bookmarkBarController_ unhighlightBookmark:bookmarkNode_];
}

- (void)cancelTracking {
  [[menuController_ menu] cancelTracking];
}

@end

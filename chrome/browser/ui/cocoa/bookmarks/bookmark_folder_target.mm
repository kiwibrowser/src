// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_folder_target.h"

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/profiles/profile_manager.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node_data.h"
#include "components/bookmarks/browser/bookmark_pasteboard_helper_mac.h"
#include "ui/base/clipboard/clipboard_util_mac.h"
#import "ui/base/cocoa/cocoa_base_utils.h"

using bookmarks::BookmarkNode;
using bookmarks::BookmarkNodeData;

NSString* kBookmarkButtonDragType = @"com.google.chrome.BookmarkButtonDrag";

@implementation BookmarkFolderTarget

- (id)initWithController:(id<BookmarkButtonControllerProtocol>)controller
                 profile:(Profile*)profile {
  if ((self = [super init])) {
    controller_ = controller;
    profile_ = profile;
  }
  return self;
}

// This IBAction is called when the user clicks (mouseUp, really) on a
// "folder" bookmark button.  (In this context, "Click" does not
// include right-click to open a context menu which follows a
// different path).  Scenarios when folder X is clicked:
//  *Predicate*        *Action*
//  (nothing)          Open Folder X
//  Folder X open      Close folder X
//  Folder Y open      Close Y, open X
//  Cmd-click          Open All with proper disposition
//
//  Note complication in which a click-drag engages drag and drop, not
//  a click-to-open.  Thus the path to get here is a little twisted.
- (IBAction)openBookmarkFolderFromButton:(id)sender {
  DCHECK(sender);
  // Watch out for a modifier click.  For example, command-click
  // should open all.
  //
  // NOTE: we cannot use [[sender cell] mouseDownFlags] because we
  // thwart the normal mouse click mechanism to make buttons
  // draggable.  Thus we must use [NSApp currentEvent].
  //
  // Holding command while using the scroll wheel (or moving around
  // over a bookmark folder) can confuse us.  Unless we check the
  // event type, we are not sure if this is an "open folder" due to a
  // hover-open or "open folder" due to a click.  It doesn't matter
  // (both do the same thing) unless a modifier is held, since
  // command-click should "open all" but command-move should not.
  // WindowOpenDispositionFromNSEvent does not consider the event
  // type; only the modifiers.  Thus the need for an extra
  // event-type-check here.
  DCHECK([sender bookmarkNode]->is_folder());
  NSEvent* event = [NSApp currentEvent];
  WindowOpenDisposition disposition =
      ui::WindowOpenDispositionFromNSEvent(event);
  if (([event type] != NSMouseEntered) && ([event type] != NSMouseMoved) &&
      ([event type] != NSScrollWheel) &&
      (disposition == WindowOpenDisposition::NEW_BACKGROUND_TAB)) {
    [controller_ closeAllBookmarkFolders];
    [controller_ openAll:[sender bookmarkNode] disposition:disposition];
    return;
  }

  // If click on same folder, close it and be done.
  // Else we clicked on a different folder so more work to do.
  if ([[controller_ folderController] parentButton] == sender) {
    [controller_ closeBookmarkFolder:controller_];
    return;
  }

  [controller_ addNewFolderControllerWithParentButton:sender];
}

- (NSPasteboardItem*)pasteboardItemForDragOfButton:(BookmarkButton*)button {
  const BookmarkNode* node = [button bookmarkNode];
  DCHECK(node);

  NSPasteboardItem* item = nil;
  if (node->is_folder()) {
    // TODO(viettrungluu): I'm not sure what we should do, so just declare the
    // "additional" types we're given for now. Maybe we want to add a list of
    // URLs? Would we then have to recurse if there were subfolders?
    item = [[[NSPasteboardItem alloc] init] autorelease];
  } else {
    BookmarkNodeData data(node);
    data.SetOriginatingProfilePath(profile_->GetPath());
    item = PasteboardItemFromBookmarks(data.elements, profile_->GetPath());
  }

  [item
      setData:[NSData dataWithBytes:&button length:sizeof(button)]
      forType:ui::ClipboardUtil::UTIForPasteboardType(kBookmarkButtonDragType)];
  return item;
}

@end

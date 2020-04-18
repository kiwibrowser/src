// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_NAME_FOLDER_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_NAME_FOLDER_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"

class BookmarkModelObserverForCocoa;
class Profile;

namespace bookmarks {
class BookmarkNode;
}

// A controller for dialog to let the user create a new folder or
// rename an existing folder.  Accessible from a context menu on a
// bookmark button or the bookmark bar.
@interface BookmarkNameFolderController : NSWindowController {
 @private
  IBOutlet NSTextField* nameField_;
  IBOutlet NSButton* okButton_;

  NSWindow* parentWindow_;  // weak
  Profile* profile_;  // weak

  // Weak; owned by the model.  Can be NULL (see below).  Either node_
  // is non-NULL (renaming a folder), or parent_ is non-NULL (adding a
  // new one).
  const bookmarks::BookmarkNode* node_;
  const bookmarks::BookmarkNode* parent_;
  int newIndex_;

  base::scoped_nsobject<NSString> initialName_;

  // Ping me when things change out from under us.
  std::unique_ptr<BookmarkModelObserverForCocoa> observer_;
}

// Use the 1st initializer for a "rename existing folder" request.
//
// Use the 2nd initializer for an "add folder" request.  If creating a
// new folder |parent| and |newIndex| specify where to put the new
// node.
- (id)initWithParentWindow:(NSWindow*)window
                   profile:(Profile*)profile
                      node:(const bookmarks::BookmarkNode*)node;
- (id)initWithParentWindow:(NSWindow*)window
                   profile:(Profile*)profile
                    parent:(const bookmarks::BookmarkNode*)parent
                  newIndex:(int)newIndex;
- (void)runAsModalSheet;
- (IBAction)cancel:(id)sender;
- (IBAction)ok:(id)sender;
@end

@interface BookmarkNameFolderController(TestingAPI)
- (NSString*)folderName;
- (void)setFolderName:(NSString*)name;
- (NSButton*)okButton;
@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_NAME_FOLDER_CONTROLLER_H_

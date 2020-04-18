// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BUBBLE_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BUBBLE_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/availability.h"
#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_model_observer_for_cocoa.h"
#import "chrome/browser/ui/cocoa/has_weak_browser_pointer.h"
#import "chrome/browser/ui/cocoa/omnibox_decoration_bubble_controller.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"

@class BookmarkBubbleController;
@class BubbleSyncPromoController;
@class DialogTextFieldEditor;

namespace bookmarks {
class BookmarkBubbleObserver;
class BookmarkModel;
class BookmarkNode;
class ManagedBookmarkService;
}

// Controller for the bookmark bubble.  The bookmark bubble is a
// bubble that pops up when clicking on the STAR next to the URL to
// add or remove it as a bookmark.  This bubble allows for editing of
// the bookmark in various ways (name, folder, etc.)
@interface BookmarkBubbleController
    : OmniboxDecorationBubbleController<NSTouchBarDelegate,
                                        HasWeakBrowserPointer> {
 @private
  // |managed_|, |model_| and |node_| are weak and owned by the current
  // browser's profile.
  bookmarks::ManagedBookmarkService* managedBookmarkService_;  // weak
  bookmarks::BookmarkModel* model_;  // weak
  const bookmarks::BookmarkNode* node_;  // weak

  // Inform the observer when the bubble is shown or closed.
  bookmarks::BookmarkBubbleObserver* bookmarkBubbleObserver_;  // weak

  BOOL alreadyBookmarked_;

  // Ping me when the bookmark model changes out from under us.
  std::unique_ptr<BookmarkModelObserverForCocoa> bookmarkObserver_;

  // Sync promo controller, if the sync promo is displayed.
  base::scoped_nsobject<BubbleSyncPromoController> syncPromoController_;

  // Field editor for |nameTextField_|.
  base::scoped_nsobject<DialogTextFieldEditor> textFieldEditor_;

  IBOutlet NSTextField* bigTitle_;   // "Bookmark" or "Bookmark Added!"
  IBOutlet NSTextField* nameTextField_;
  IBOutlet NSPopUpButton* folderPopUpButton_;
  IBOutlet NSView* syncPromoPlaceholder_;
  IBOutlet NSView* fieldLabelsContainer_;
  IBOutlet NSView* trailingButtonContainer_;
}

@property(readonly, nonatomic) const bookmarks::BookmarkNode* node;

// |node| is the bookmark node we edit in this bubble.
// |alreadyBookmarked| tells us if the node was bookmarked before the
//   user clicked on the star.  (if NO, this is a brand new bookmark).
// The owner of this object is responsible for showing the bubble if
// it desires it to be visible on the screen.  It is not shown by the
// init routine.  Closing of the window happens implicitly on dealloc.
- (id)initWithParentWindow:(NSWindow*)parentWindow
            bubbleObserver:(bookmarks::BookmarkBubbleObserver*)bubbleObserver
                   managed:(bookmarks::ManagedBookmarkService*)managed
                     model:(bookmarks::BookmarkModel*)model
                      node:(const bookmarks::BookmarkNode*)node
         alreadyBookmarked:(BOOL)alreadyBookmarked;

// Actions for buttons in the dialog.
- (IBAction)ok:(id)sender;
- (IBAction)remove:(id)sender;
- (IBAction)cancel:(id)sender;

// These actions send a -editBookmarkNode: action up the responder chain.
- (IBAction)edit:(id)sender;
- (IBAction)folderChanged:(id)sender;

// Overridden to customize the touch bar.
- (NSTouchBar*)makeTouchBar API_AVAILABLE(macos(10.12.2));

@end


// Exposed only for unit testing.
@interface BookmarkBubbleController (ExposedForUnitTesting)

@property(nonatomic, readonly) NSView* syncPromoPlaceholder;
@property(nonatomic, readonly)
    bookmarks::BookmarkBubbleObserver* bookmarkBubbleObserver;

- (void)addFolderNodes:(const bookmarks::BookmarkNode*)parent
         toPopUpButton:(NSPopUpButton*)button
           indentation:(int)indentation;
- (void)setTitle:(NSString*)title
    parentFolder:(const bookmarks::BookmarkNode*)parent;
- (void)setParentFolderSelection:(const bookmarks::BookmarkNode*)parent;
+ (NSString*)chooseAnotherFolderString;
- (NSPopUpButton*)folderPopUpButton;
@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BUBBLE_CONTROLLER_H_

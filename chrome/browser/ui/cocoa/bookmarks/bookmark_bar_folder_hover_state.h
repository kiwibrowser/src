// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_HOVER_STATE_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_HOVER_STATE_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"

// Hover state machine.  Encapsulates the hover state for
// BookmarkBarFolderController.
// A strict call order is implied with these calls.  It is ONLY valid to make
// the following state transitions:
// From:            To:               Via:
// closed           opening           scheduleOpen...:
// opening          closed            cancelPendingOpen...: or
//                  open              scheduleOpen...: completes.
// open             closing           scheduleClose...:
// closing          open              cancelPendingClose...: or
//                  closed            scheduleClose...: completes.
//
@interface BookmarkBarFolderHoverState : NSObject {
 @private
  // Enumeration of the valid states that the |hoverButton_| member can be in.
  // Because the opening and closing of hover views can be done asyncronously
  // there are periods where the hover state is in transtion between open and
  // closed.  During those times of transition the opening or closing operation
  // can be cancelled.  We serialize the opening and closing of the
  // |hoverButton_| using this state information.  This serialization is to
  // avoid race conditions where one hover button is being opened while another
  // is closing.
  enum HoverState {
    kHoverStateClosed = 0,
    kHoverStateOpening = 1,
    kHoverStateOpen = 2,
    kHoverStateClosing = 3
  };

  // Like normal menus, hovering over a folder button causes it to
  // open.  This variable is set when a hover is initiated (but has
  // not necessarily fired yet).
  base::scoped_nsobject<BookmarkButton> hoverButton_;

  // We model hover state as a state machine with specific allowable
  // transitions.  |hoverState_| is the state of this machine at any
  // given time.
  HoverState hoverState_;
}

// Designated initializer.
- (id)init;

// The BookmarkBarFolderHoverState decides when it is appropriate to hide
// and show the button that the BookmarkBarFolderController drags over.
- (NSDragOperation)draggingEnteredButton:(BookmarkButton*)button;

// The BookmarkBarFolderHoverState decides the fate of the hover button
// when the BookmarkBarFolderController's view is exited.
- (void)draggingExited;

@end

// Exposing these for unit testing purposes.  They are used privately in the
// implementation as well.
@interface BookmarkBarFolderHoverState(PrivateAPI)
// State change APIs.
- (void)scheduleCloseBookmarkFolderOnHoverButton;
- (void)cancelPendingCloseBookmarkFolderOnHoverButton;
- (void)scheduleOpenBookmarkFolderOnHoverButton:(BookmarkButton*)hoverButton;
- (void)cancelPendingOpenBookmarkFolderOnHoverButton;
@end

// Exposing these for unit testing purposes.  They are used only in tests.
@interface BookmarkBarFolderHoverState(TestingAPI)
// Accessors and setters for button and hover state.
- (BookmarkButton*)hoverButton;
- (HoverState)hoverState;
@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_HOVER_STATE_H_

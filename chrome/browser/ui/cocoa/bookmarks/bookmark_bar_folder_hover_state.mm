// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_hover_state.h"

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"

@interface BookmarkBarFolderHoverState(Private)
- (void)setHoverState:(HoverState)state;
- (void)closeBookmarkFolderOnHoverButton:(BookmarkButton*)button;
- (void)openBookmarkFolderOnHoverButton:(BookmarkButton*)button;
@end

@implementation BookmarkBarFolderHoverState

- (id)init {
  if ((self = [super init])) {
    hoverState_ = kHoverStateClosed;
  }
  return self;
}

- (NSDragOperation)draggingEnteredButton:(BookmarkButton*)button {
  if ([button isFolder]) {
    if (hoverButton_ == button) {
      // CASE A: hoverButton_ == button implies we've dragged over
      // the same folder so no need to open or close anything new.
    } else if (hoverButton_ &&
               hoverButton_ != button) {
      // CASE B: we have a hoverButton_ but it is different from the new button.
      // This implies we've dragged over a new folder, so we'll close the old
      // and open the new.
      // Note that we only schedule the open or close if we have no other tasks
      // currently pending.

      // Since the new bookmark folder is not opened until the previous is
      // closed, the NSDraggingDestination must provide continuous callbacks,
      // even if the cursor isn't moving.
      if (hoverState_ == kHoverStateOpen) {
        // Close the old.
        [self scheduleCloseBookmarkFolderOnHoverButton];
      } else if (hoverState_ == kHoverStateClosed) {
        // Open the new.
        [self scheduleOpenBookmarkFolderOnHoverButton:button];
      }
    } else if (!hoverButton_) {
      // CASE C: we don't have a current hoverButton_ but we have dragged onto
      // a new folder so we open the new one.
      [self scheduleOpenBookmarkFolderOnHoverButton:button];
    }
  } else if (!button) {
    if (hoverButton_) {
      // CASE D: We have a hoverButton_ but we've moved onto an area that
      // requires no hover.  We close the hoverButton_ in this case.  This
      // means cancelling if the open is pending (i.e. |kHoverStateOpening|)
      // or closing if we don't alrealy have once in progress.

      // Intiate close only if we have not already done so.
      if (hoverState_ == kHoverStateOpening) {
        // Cancel the pending open.
        [self cancelPendingOpenBookmarkFolderOnHoverButton];
      } else if (hoverState_ != kHoverStateClosing) {
        // Schedule the close.
        [self scheduleCloseBookmarkFolderOnHoverButton];
      }
    } else {
      // CASE E: We have neither a hoverButton_ nor a new button that requires
      // a hover.  In this case we do nothing.
    }
  }

  return NSDragOperationMove;
}

- (void)draggingExited {
  if (hoverButton_) {
    if (hoverState_ == kHoverStateOpening) {
      [self cancelPendingOpenBookmarkFolderOnHoverButton];
    } else if (hoverState_ == kHoverStateClosing) {
      [self cancelPendingCloseBookmarkFolderOnHoverButton];
    }
  }
}

// Schedule close of hover button.  Transition to kHoverStateClosing state.
- (void)scheduleCloseBookmarkFolderOnHoverButton {
  DCHECK(hoverButton_);
  [self setHoverState:kHoverStateClosing];
  [self performSelector:@selector(closeBookmarkFolderOnHoverButton:)
             withObject:hoverButton_
             afterDelay:bookmarks::kDragHoverCloseDelay
                inModes:[NSArray arrayWithObject:NSRunLoopCommonModes]];
}

// Cancel pending hover close.  Transition to kHoverStateOpen state.
- (void)cancelPendingCloseBookmarkFolderOnHoverButton {
  [self setHoverState:kHoverStateOpen];
  [NSObject
      cancelPreviousPerformRequestsWithTarget:self
      selector:@selector(closeBookmarkFolderOnHoverButton:)
      object:hoverButton_];
}

// Schedule open of hover button.  Transition to kHoverStateOpening state.
- (void)scheduleOpenBookmarkFolderOnHoverButton:(BookmarkButton*)button {
  DCHECK(button);
  hoverButton_.reset([button retain]);
  [self setHoverState:kHoverStateOpening];
  [self performSelector:@selector(openBookmarkFolderOnHoverButton:)
             withObject:hoverButton_
             afterDelay:bookmarks::kDragHoverOpenDelay
                inModes:[NSArray arrayWithObject:NSRunLoopCommonModes]];
}

// Cancel pending hover open.  Transition to kHoverStateClosed state.
- (void)cancelPendingOpenBookmarkFolderOnHoverButton {
  [self setHoverState:kHoverStateClosed];
  [NSObject
      cancelPreviousPerformRequestsWithTarget:self
      selector:@selector(openBookmarkFolderOnHoverButton:)
      object:hoverButton_];
  hoverButton_.reset();
}

// Hover button accessor.  For testing only.
- (BookmarkButton*)hoverButton {
  return hoverButton_;
}

// Hover state accessor.  For testing only.
- (HoverState)hoverState {
  return hoverState_;
}

// This method encodes the rules of our |hoverButton_| state machine.  Only
// specific state transitions are allowable (encoded in the DCHECK).
// Note that there is no state for simultaneously opening and closing.  A
// pending open must complete before scheduling a close, and vice versa.  And
// it is not possible to make a transition directly from open to closed, and
// vice versa.
- (void)setHoverState:(HoverState)state {
  DCHECK(
    (hoverState_ == kHoverStateClosed && state == kHoverStateOpening) ||
    (hoverState_ == kHoverStateOpening && state == kHoverStateClosed) ||
    (hoverState_ == kHoverStateOpening && state == kHoverStateOpen) ||
    (hoverState_ == kHoverStateOpen && state == kHoverStateClosing) ||
    (hoverState_ == kHoverStateClosing && state == kHoverStateOpen) ||
    (hoverState_ == kHoverStateClosing && state == kHoverStateClosed)
  ) << "bad transition: old = " << hoverState_ << " new = " << state;

  hoverState_ = state;
}

// Called after a delay to close a previously hover-opened folder.
// Note: this method is not meant to be invoked directly, only through
// a delayed call to |scheduleCloseBookmarkFolderOnHoverButton:|.
- (void)closeBookmarkFolderOnHoverButton:(BookmarkButton*)button {
  [NSObject
      cancelPreviousPerformRequestsWithTarget:self
      selector:@selector(closeBookmarkFolderOnHoverButton:)
      object:hoverButton_];
  [self setHoverState:kHoverStateClosed];
  [[button target] closeBookmarkFolder:button];
  hoverButton_.reset();
}

// Called after a delay to open a new hover folder.
// Note: this method is not meant to be invoked directly, only through
// a delayed call to |scheduleOpenBookmarkFolderOnHoverButton:|.
- (void)openBookmarkFolderOnHoverButton:(BookmarkButton*)button {
  [self setHoverState:kHoverStateOpen];
  [[button target] performSelector:@selector(openBookmarkFolderFromButton:)
      withObject:button];
}

@end

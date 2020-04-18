// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BUBBLE_OBSERVER_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BUBBLE_OBSERVER_COCOA_H_

#include "base/macros.h"
#include "chrome/browser/ui/bookmarks/bookmark_bubble_observer.h"

#import "base/mac/scoped_nsobject.h"

@class BrowserWindowController;

// Manages toolbar visibility locking and bookmark button pulsing.
class BookmarkBubbleObserverCocoa : public bookmarks::BookmarkBubbleObserver {
 public:
  explicit BookmarkBubbleObserverCocoa(BrowserWindowController* controller);
  ~BookmarkBubbleObserverCocoa() override;

  // bookmarks::BookmarkBubbleObserver.
  void OnBookmarkBubbleShown(const bookmarks::BookmarkNode* node) override;
  void OnBookmarkBubbleHidden() override;

 private:
  BrowserWindowController* controller_;  // Weak, owns us.
  base::scoped_nsobject<NSObject> lockOwner_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkBubbleObserverCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BUBBLE_OBSERVER_COCOA_H_

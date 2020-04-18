// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_BROWSER_STATE_BOOKMARK_MODEL_LOADED_OBSERVER_H_
#define IOS_CHROME_BROWSER_BROWSER_STATE_BOOKMARK_MODEL_LOADED_OBSERVER_H_

#include "base/macros.h"
#include "components/bookmarks/browser/base_bookmark_model_observer.h"

namespace ios {
class ChromeBrowserState;
}

class BookmarkModelLoadedObserver
    : public bookmarks::BaseBookmarkModelObserver {
 public:
  explicit BookmarkModelLoadedObserver(ios::ChromeBrowserState* browser_state);

 private:
  void BookmarkModelChanged() override;
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
  void BookmarkModelBeingDeleted(bookmarks::BookmarkModel* model) override;

  ios::ChromeBrowserState* browser_state_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkModelLoadedObserver);
};

#endif  // IOS_CHROME_BROWSER_BROWSER_STATE_BOOKMARK_MODEL_LOADED_OBSERVER_H_

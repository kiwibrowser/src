// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_BAR_INSTRUCTIONS_DELEGATE_H_
#define CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_BAR_INSTRUCTIONS_DELEGATE_H_

// The delegate is notified once the user clicks on the link to import
// bookmarks.
class BookmarkBarInstructionsDelegate {
 public:
  virtual void OnImportBookmarks() = 0;

 protected:
  virtual ~BookmarkBarInstructionsDelegate() {}
};

#endif  // CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_BAR_INSTRUCTIONS_DELEGATE_H_

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_TAB_HELPER_DELEGATE_H_
#define CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_TAB_HELPER_DELEGATE_H_


namespace content {
class WebContents;
}

// Objects implement this interface to get notified about changes in the
// BookmarkTabHelper and to provide necessary functionality.
class BookmarkTabHelperDelegate {
 public:
  // Notification that the starredness of the current URL changed.
  virtual void URLStarredChanged(content::WebContents* web_contents,
                                 bool starred) = 0;

 protected:
  virtual ~BookmarkTabHelperDelegate();
};

#endif  // CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_TAB_HELPER_DELEGATE_H_

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BOOKMARKS_BOOKMARK_STATS_H_
#define CHROME_BROWSER_BOOKMARKS_BOOKMARK_STATS_H_

namespace bookmarks {
class BookmarkNode;
}

// This enum is used for the Bookmarks.EntryPoint histogram.
enum BookmarkEntryPoint {
  BOOKMARK_ENTRY_POINT_ACCELERATOR,
  BOOKMARK_ENTRY_POINT_STAR_GESTURE,
  BOOKMARK_ENTRY_POINT_STAR_KEY,
  BOOKMARK_ENTRY_POINT_STAR_MOUSE,

  BOOKMARK_ENTRY_POINT_LIMIT // Keep this last.
};

// This enum is used for the Bookmarks.LaunchLocation histogram.
enum BookmarkLaunchLocation {
  BOOKMARK_LAUNCH_LOCATION_NONE,
  BOOKMARK_LAUNCH_LOCATION_ATTACHED_BAR = 0,
  BOOKMARK_LAUNCH_LOCATION_DETACHED_BAR,
  // These two are kind of sub-categories of the bookmark bar. Generally
  // a launch from a context menu or subfolder could be classified in one of
  // the other two bar buckets, but doing so is difficult because the menus
  // don't know of their greater place in Chrome.
  BOOKMARK_LAUNCH_LOCATION_BAR_SUBFOLDER,
  BOOKMARK_LAUNCH_LOCATION_CONTEXT_MENU,

  // Bookmarks menu within app menu.
  BOOKMARK_LAUNCH_LOCATION_APP_MENU,
  // Bookmark manager.
  BOOKMARK_LAUNCH_LOCATION_MANAGER,
  // Autocomplete suggestion.
  BOOKMARK_LAUNCH_LOCATION_OMNIBOX,

  BOOKMARK_LAUNCH_LOCATION_LIMIT  // Keep this last.
};

// Records the launch of a bookmark for UMA purposes.
void RecordBookmarkLaunch(const bookmarks::BookmarkNode* node,
                          BookmarkLaunchLocation location);

// Records the user opening a folder of bookmarks for UMA purposes.
void RecordBookmarkFolderOpen(BookmarkLaunchLocation location);

// Records the user opening the apps page for UMA purposes.
void RecordBookmarkAppsPageOpen(BookmarkLaunchLocation location);

#endif  // CHROME_BROWSER_BOOKMARKS_BOOKMARK_STATS_H_

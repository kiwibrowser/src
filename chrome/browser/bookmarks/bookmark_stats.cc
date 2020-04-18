// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/bookmarks/bookmark_stats.h"

#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "components/bookmarks/browser/bookmark_model.h"

using bookmarks::BookmarkNode;

void RecordBookmarkLaunch(const BookmarkNode* node,
                          BookmarkLaunchLocation location) {
  if (location == BOOKMARK_LAUNCH_LOCATION_DETACHED_BAR ||
      location == BOOKMARK_LAUNCH_LOCATION_ATTACHED_BAR) {
    base::RecordAction(base::UserMetricsAction("ClickedBookmarkBarURLButton"));
  }
  UMA_HISTOGRAM_ENUMERATION(
      "Bookmarks.LaunchLocation", location, BOOKMARK_LAUNCH_LOCATION_LIMIT);

  if (!node)
    return;

  // In the cases where a bookmark node is provided, record the depth of the
  // bookmark in the tree.
  int depth = 0;
  for (const BookmarkNode* iter = node; iter != NULL; iter = iter->parent()) {
    depth++;
  }
  // Record |depth - 2| to offset the invisible root node and permanent nodes
  // (Bookmark Bar, Mobile Bookmarks or Other Bookmarks)
  UMA_HISTOGRAM_COUNTS("Bookmarks.LaunchDepth", depth - 2);
}

void RecordBookmarkFolderOpen(BookmarkLaunchLocation location) {
  if (location == BOOKMARK_LAUNCH_LOCATION_DETACHED_BAR ||
      location == BOOKMARK_LAUNCH_LOCATION_ATTACHED_BAR) {
    base::RecordAction(base::UserMetricsAction("ClickedBookmarkBarFolder"));
  }
}

void RecordBookmarkAppsPageOpen(BookmarkLaunchLocation location) {
  if (location == BOOKMARK_LAUNCH_LOCATION_DETACHED_BAR ||
      location == BOOKMARK_LAUNCH_LOCATION_ATTACHED_BAR) {
    base::RecordAction(
        base::UserMetricsAction("ClickedBookmarkBarAppsShortcutButton"));
  }
}

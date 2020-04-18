// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_BOOKMARKS_BOOKMARK_LAST_VISIT_UTILS_H_
#define COMPONENTS_NTP_SNIPPETS_BOOKMARKS_BOOKMARK_LAST_VISIT_UTILS_H_

#include <vector>

#include "base/callback.h"

class GURL;

namespace base {
class Time;
}  // namespace base

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}  // namespace bookmarks

namespace ntp_snippets {

// If there is a bookmark for |url|, this function updates its last visit date
// to now. If there are multiple bookmarks for a given URL, it updates all of
// them. The last visit dates are kept separate for mobile and desktop,
// according to |is_mobile_platform|.
void UpdateBookmarkOnURLVisitedInMainFrame(
    bookmarks::BookmarkModel* bookmark_model,
    const GURL& url,
    bool is_mobile_platform);

// Reads the last visit date for a given bookmark |node|. On success, |out| is
// set to the extracted time and 'true' is returned. Otherwise returns false
// which also includes the case that the bookmark was dismissed from the NTP.
// As visits, we primarily understand visits on Android (the visit when the
// bookmark is created also counts). Visits on desktop platforms are considered
// only if |consider_visits_from_desktop|.
bool GetLastVisitDateForNTPBookmark(const bookmarks::BookmarkNode& node,
                                    bool consider_visits_from_desktop,
                                    base::Time* out);

// Marks all bookmarks with the given URL as dismissed.
void MarkBookmarksDismissed(bookmarks::BookmarkModel* bookmark_model,
                            const GURL& url);

// Gets the dismissed flag for a given bookmark |node|. Defaults to false.
bool IsDismissedFromNTPForBookmark(const bookmarks::BookmarkNode& node);

// Removes the dismissed flag from all bookmarks (only for debugging).
void MarkAllBookmarksUndismissed(bookmarks::BookmarkModel* bookmark_model);

// Returns the list of most recently visited, non-dismissed bookmarks.
// For each bookmarked URL, it returns the most recently created bookmark.
// The result is ordered by visit time (the most recent first). Only bookmarks
// visited after |min_visit_time| are considered, at most |max_count| bookmarks
// are returned.
// If |consider_visits_from_desktop|, also visits to bookmarks on synced desktop
// platforms are considered (and not only on this and other synced Android
// devices).
std::vector<const bookmarks::BookmarkNode*> GetRecentlyVisitedBookmarks(
    bookmarks::BookmarkModel* bookmark_model,
    int max_count,
    const base::Time& min_visit_time,
    bool consider_visits_from_desktop);

// Returns the list of all dismissed bookmarks. Only used for debugging.
std::vector<const bookmarks::BookmarkNode*> GetDismissedBookmarksForDebugging(
    bookmarks::BookmarkModel* bookmark_model);

// Removes last-visited data (incl. any other metadata managed by content
// suggestions) for bookmarks within the provided time range.
// TODO(tschumann): Implement URL filtering.
void RemoveLastVisitedDatesBetween(const base::Time& begin,
                                   const base::Time& end,
                                   base::Callback<bool(const GURL& url)> filter,
                                   bookmarks::BookmarkModel* bookmark_model);

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_BOOKMARKS_BOOKMARK_LAST_VISIT_UTILS_H_

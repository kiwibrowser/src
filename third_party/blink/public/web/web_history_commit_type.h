// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_HISTORY_COMMIT_TYPE_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_HISTORY_COMMIT_TYPE_H_

namespace blink {

enum WebHistoryCommitType {
  // The default case (link clicks, user-typed urls, etc.), appends
  // a new history entry to the back/forward list.
  kWebStandardCommit,
  // A load that originated from history, whether from the
  // back/forward list or session restore. The back/forward list is
  // not modified, but our position in the list is.
  kWebBackForwardCommit,
  // Each history entry is represented by a tree, where each node
  // contains the history state of a single frame in the page. Initial
  // loads in a new subframe append a new node to that tree.
  kWebInitialCommitInChildFrame,
  // Reloads, client redirects, etc. Loads that neither originate from
  // nor add entries to the back/forward list.
  kWebHistoryInertCommit
};

}  // namespace blink

#endif

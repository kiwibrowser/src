// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/bookmarks/browser/bookmark_node_data.h"

#include "components/bookmarks/browser/bookmark_pasteboard_helper_mac.h"

namespace bookmarks {

// static
bool BookmarkNodeData::ClipboardContainsBookmarks() {
  return PasteboardContainsBookmarks(ui::CLIPBOARD_TYPE_COPY_PASTE);
}

void BookmarkNodeData::WriteToClipboard(ui::ClipboardType type) {
  WriteBookmarksToPasteboard(type, elements, profile_path_);
}

bool BookmarkNodeData::ReadFromClipboard(ui::ClipboardType type) {
  base::FilePath file_path;
  if (ReadBookmarksFromPasteboard(type, elements, &file_path)) {
    profile_path_ = file_path;
    return true;
  }

  return false;
}

}  // namespace bookmarks

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_PASTEBOARD_HELPER_MAC_H_
#define COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_PASTEBOARD_HELPER_MAC_H_

#include "components/bookmarks/browser/bookmark_node_data.h"

#if defined(__OBJC__)
@class NSPasteboardItem;
@class NSString;
#endif  // __OBJC__

namespace base {
class FilePath;
}

namespace bookmarks {

// This set of functions lets C++ code interact with the cocoa pasteboard and
// dragging methods.

#if defined(__OBJC__)
// Creates a NSPasteboardItem that contains all the data for the bookmarks.
NSPasteboardItem* PasteboardItemFromBookmarks(
    const std::vector<BookmarkNodeData::Element>& elements,
    const base::FilePath& profile_path);
#endif  // __OBJC__

// Writes a set of bookmark elements from a profile to the specified pasteboard.
void WriteBookmarksToPasteboard(
    ui::ClipboardType type,
    const std::vector<BookmarkNodeData::Element>& elements,
    const base::FilePath& profile_path);

// Reads a set of bookmark elements from the specified pasteboard.
bool ReadBookmarksFromPasteboard(
    ui::ClipboardType type,
    std::vector<BookmarkNodeData::Element>& elements,
    base::FilePath* profile_path);

// Returns true if the specified pasteboard contains any sort of bookmark
// elements. It currently does not consider a plaintext url a valid bookmark.
bool PasteboardContainsBookmarks(ui::ClipboardType type);

}  // namespace bookmarks

#if defined(__OBJC__)
// Pasteboard type for dictionary containing bookmark structure consisting
// of individual bookmark nodes and/or bookmark folders.
extern "C" NSString* const kBookmarkDictionaryListPboardType;
#endif  // __OBJC__

#endif  // COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_PASTEBOARD_HELPER_MAC_H_

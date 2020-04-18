// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_CONSTANTS_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_CONSTANTS_H_

#include <ApplicationServices/ApplicationServices.h>

// Constants used for positioning the bookmark bar. These aren't placed in a
// different file because they're conditionally included in cross platform code
// and thus no Objective-C++ stuff.
namespace bookmarks {

// Correction used for computing other values based on the height.
const int kMaterialVisualHeightOffset = 2;

// The height of buttons in a bookmark bar folder menu.
const CGFloat kBookmarkFolderButtonHeight = 24.0;

// The radius of the corner curves on the menu. Also used for sizing the shadow
// window behind the menu window at times when the menu can be scrolled.
const CGFloat kBookmarkBarMenuCornerRadius = 4.0;

// Overlap (in pixels) between the toolbar and the bookmark bar (when showing in
// normal mode).
const CGFloat kBookmarkBarOverlap = 3.0;

}  // namespace bookmarks

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_CONSTANTS_H_

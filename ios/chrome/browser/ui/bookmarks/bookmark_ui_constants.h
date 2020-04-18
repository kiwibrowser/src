// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_UI_CONSTANTS_H_
#define IOS_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_UI_CONSTANTS_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

// UIToolbar accessibility constants.

// Accessibility identifier of the BookmarkEditVC toolbar delete button.
extern NSString* const kBookmarkEditDeleteButtonIdentifier;
// Accessibility identifier of the BookmarkFolderEditorVC toolbar delete button.
extern NSString* const kBookmarkFolderEditorDeleteButtonIdentifier;
// Accessibility identifier of the BookmarkHomeVC leading button.
extern NSString* const kBookmarkHomeLeadingButtonIdentifier;
// Accessibility identifier of the BookmarkHomeVC center button.
extern NSString* const kBookmarkHomeCenterButtonIdentifier;
// Accessibility identifier of the BookmarkHomeVC trailing button.
extern NSString* const kBookmarkHomeTrailingButtonIdentifier;
// Accessibility identifier of the BookmarkHomeVC UIToolbar.
extern NSString* const kBookmarkHomeUIToolbarIdentifier;

// Cell Layout constants.

// The space between UIViews inside the cell.
extern const CGFloat kBookmarkCellViewSpacing;
// The vertical space between the Cell margin and its contents.
extern const CGFloat kBookmarkCellVerticalInset;
// The horizontal leading space between the Cell margin and its contents.
extern const CGFloat kBookmarkCellHorizontalLeadingInset;
// The horizontal trailing space between the Cell margin and its contents.
extern const CGFloat kBookmarkCellHorizontalTrailingInset;
// The horizontal space between the Cell content and its accessory view.
extern const CGFloat kBookmarkCellHorizontalAccessoryViewSpacing;

// Cell accessibility constants.

// Accessibility identifier of the BookmarkHomeVC UIToolbar.
extern NSString* const kBookmarkCreateNewFolderCellIdentifier;

#endif  // IOS_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARK_UI_CONSTANTS_H_

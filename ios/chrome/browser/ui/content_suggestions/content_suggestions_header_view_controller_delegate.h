// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_VIEW_CONTROLLER_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_VIEW_CONTROLLER_DELEGATE_H_

#import <UIKit/UIKit.h>

// Delegate for the ContentSuggestionsHeaderViewController.
@protocol ContentSuggestionsHeaderViewControllerDelegate

// Returns whether a context menu is visible.
- (BOOL)isContextMenuVisible;

// Returns whether the collection is scrolled to its top.
- (BOOL)isScrolledToTop;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_VIEW_CONTROLLER_DELEGATE_H_

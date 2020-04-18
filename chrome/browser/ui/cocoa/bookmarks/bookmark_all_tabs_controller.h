// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_ALL_TABS_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_ALL_TABS_CONTROLLER_H_

#include <utility>
#include <vector>

#include "base/strings/string16.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_editor_base_controller.h"

// A list of pairs containing the name and URL associated with each
// currently active tab in the active browser window.
typedef std::pair<base::string16, GURL> ActiveTabNameURLPair;
typedef std::vector<ActiveTabNameURLPair> ActiveTabsNameURLPairVector;

// A controller for the Bookmark All Tabs sheet which is presented upon
// selecting the Bookmark All Tabs... menu item shown by the contextual
// menu in the bookmarks bar.
@interface BookmarkAllTabsController : BookmarkEditorBaseController {
 @private
  ActiveTabsNameURLPairVector activeTabPairsVector_;
}

- (id)initWithParentWindow:(NSWindow*)parentWindow
                   profile:(Profile*)profile
                    parent:(const bookmarks::BookmarkNode*)parent
                       url:(const GURL&)url
                     title:(const base::string16&)title
             configuration:(BookmarkEditor::Configuration)configuration;

@end

@interface BookmarkAllTabsController(TestingAPI)

// Initializes the list of all tab names and URLs.  Overridden by unit test
// to provide canned test data.
- (void)UpdateActiveTabPairs;

// Provides testing access to tab pairs list.
- (ActiveTabsNameURLPairVector*)activeTabPairsVector;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_ALL_TABS_CONTROLLER_H_

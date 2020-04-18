// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_all_tabs_controller.h"

#include "base/strings/string16.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/cocoa/last_active_browser_cocoa.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util_mac.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using content::WebContents;

@implementation BookmarkAllTabsController

- (id)initWithParentWindow:(NSWindow*)parentWindow
                   profile:(Profile*)profile
                    parent:(const BookmarkNode*)parent
                       url:(const GURL&)url
                     title:(const base::string16&)title
             configuration:(BookmarkEditor::Configuration)configuration {
  NSString* nibName = @"BookmarkAllTabs";
  if ((self = [super initWithParentWindow:parentWindow
                                  nibName:nibName
                                  profile:profile
                                   parent:parent
                                      url:url
                                    title:title
                            configuration:configuration])) {
  }
  return self;
}

- (void)awakeFromNib {
  [self setInitialName:
      l10n_util::GetNSStringWithFixup(IDS_BOOKMARK_EDITOR_NEW_FOLDER_NAME)];
  [super awakeFromNib];
}

#pragma mark Bookmark Editing

- (void)UpdateActiveTabPairs {
  activeTabPairsVector_.clear();
  Browser* browser = chrome::GetLastActiveBrowser();
  const int tabCount = browser->tab_strip_model()->count();
  for (int i = 0; i < tabCount; ++i) {
    WebContents* contents = browser->tab_strip_model()->GetWebContentsAt(i);
    ActiveTabNameURLPair tabPair(contents->GetTitle(), contents->GetURL());
    activeTabPairsVector_.push_back(tabPair);
  }
}

// Called by -[BookmarkEditorBaseController ok:].  Creates the container
// folder for the tabs and then the bookmarks in that new folder.
// Returns a BOOL as an NSNumber indicating that the commit may proceed.
- (NSNumber*)didCommit {
  const BookmarkNode* newParentNode = [self selectedNode];
  if (!newParentNode)
    return [NSNumber numberWithBool:NO];
  int newIndex = newParentNode->child_count();
  // Create the new folder which will contain all of the tab URLs.
  NSString* newFolderName = [self displayName];
  base::string16 newFolderString = base::SysNSStringToUTF16(newFolderName);
  BookmarkModel* model = [self bookmarkModel];
  const BookmarkNode* newFolder = model->AddFolder(newParentNode, newIndex,
                                                   newFolderString);
  // Get a list of all open tabs, create nodes for them, and add
  // to the new folder node.
  [self UpdateActiveTabPairs];
  int i = 0;
  for (ActiveTabsNameURLPairVector::const_iterator it =
           activeTabPairsVector_.begin();
       it != activeTabPairsVector_.end(); ++it, ++i) {
    model->AddURL(newFolder, i, it->first, it->second);
  }
  return [NSNumber numberWithBool:YES];
}

- (ActiveTabsNameURLPairVector*)activeTabPairsVector {
  return &activeTabPairsVector_;
}

@end  // BookmarkAllTabsController


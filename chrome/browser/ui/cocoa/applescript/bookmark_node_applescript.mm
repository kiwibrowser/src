// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/applescript/bookmark_node_applescript.h"

#include "base/logging.h"
#import "base/mac/foundation_util.h"
#import "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/app_controller_mac.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#import "chrome/browser/chrome_browser_application_mac.h"
#include "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/applescript/bookmark_item_applescript.h"
#import "chrome/browser/ui/cocoa/applescript/error_applescript.h"
#include "components/bookmarks/browser/bookmark_model.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

@interface BookmarkNodeAppleScript()
@property (nonatomic, copy) NSString* tempTitle;
@end

@implementation BookmarkNodeAppleScript

@synthesize tempTitle = tempTitle_;

- (id)init {
  if ((self = [super init])) {
    BookmarkModel* model = [self bookmarkModel];
    if (!model) {
      [self release];
      return nil;
    }

    base::scoped_nsobject<NSNumber> numID(
        [[NSNumber alloc] initWithLongLong:model->next_node_id()]);
    [self setUniqueID:numID];
    [self setTempTitle:@""];
  }
  return self;
}

- (void)dealloc {
  [tempTitle_ release];
  [super dealloc];
}


- (id)initWithBookmarkNode:(const BookmarkNode*)aBookmarkNode {
  if (!aBookmarkNode) {
    [self release];
    return nil;
  }

  if ((self = [super init])) {
    // It is safe to be weak, if a bookmark item/folder goes away
    // (eg user deleting a folder) the applescript runtime calls
    // bookmarkFolders/bookmarkItems in BookmarkFolderAppleScript
    // and this particular bookmark item/folder is never returned.
    bookmarkNode_ = aBookmarkNode;

    base::scoped_nsobject<NSNumber> numID(
        [[NSNumber alloc] initWithLongLong:aBookmarkNode->id()]);
    [self setUniqueID:numID];
  }
  return self;
}

- (void)setBookmarkNode:(const BookmarkNode*)aBookmarkNode {
  DCHECK(aBookmarkNode);
  // It is safe to be weak, if a bookmark item/folder goes away
  // (eg user deleting a folder) the applescript runtime calls
  // bookmarkFolders/bookmarkItems in BookmarkFolderAppleScript
  // and this particular bookmark item/folder is never returned.
  bookmarkNode_ = aBookmarkNode;

  base::scoped_nsobject<NSNumber> numID(
      [[NSNumber alloc] initWithLongLong:aBookmarkNode->id()]);
  [self setUniqueID:numID];

  [self setTitle:[self tempTitle]];
}

- (NSString*)title {
  if (!bookmarkNode_)
    return tempTitle_;

  return base::SysUTF16ToNSString(bookmarkNode_->GetTitle());
}

- (void)setTitle:(NSString*)aTitle {
  // If the scripter enters |make new bookmarks folder with properties
  // {title:"foo"}|, the node has not yet been created so title is stored in the
  // temp title.
  if (!bookmarkNode_) {
    [self setTempTitle:aTitle];
    return;
  }

  BookmarkModel* model = [self bookmarkModel];
  if (!model)
    return;

  model->SetTitle(bookmarkNode_, base::SysNSStringToUTF16(aTitle));
}

- (NSNumber*)index {
  const BookmarkNode* parent = bookmarkNode_->parent();
  int index = parent->GetIndexOf(bookmarkNode_);
  // NOTE: AppleScript is 1-Based.
  return [NSNumber numberWithInt:index+1];
}

- (BookmarkModel*)bookmarkModel {
  AppController* appDelegate =
      base::mac::ObjCCastStrict<AppController>([NSApp delegate]);

  Profile* lastProfile = [appDelegate lastProfile];
  if (!lastProfile) {
    AppleScript::SetError(AppleScript::errGetProfile);
    return NULL;
  }

  BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(lastProfile);
  if (!model->loaded()) {
    AppleScript::SetError(AppleScript::errBookmarkModelLoad);
    return NULL;
  }

  return model;
}

@end

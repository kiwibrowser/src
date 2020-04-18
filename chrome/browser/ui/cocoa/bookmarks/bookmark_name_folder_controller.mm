// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_name_folder_controller.h"

#include "base/mac/bundle_locations.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/cocoa/bookmarks/bookmark_model_observer_for_cocoa.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

@implementation BookmarkNameFolderController

// Common initializer (private).
- (id)initWithParentWindow:(NSWindow*)window
                   profile:(Profile*)profile
                      node:(const BookmarkNode*)node
                    parent:(const BookmarkNode*)parent
                  newIndex:(int)newIndex {
  NSString* nibpath = [base::mac::FrameworkBundle()
                        pathForResource:@"BookmarkNameFolder"
                        ofType:@"nib"];
  if ((self = [super initWithWindowNibPath:nibpath owner:self])) {
    parentWindow_ = window;
    profile_ = profile;
    node_ = node;
    parent_ = parent;
    newIndex_ = newIndex;
    if (parent) {
      DCHECK_LE(newIndex, parent->child_count());
    }
    if (node_) {
      initialName_.reset([base::SysUTF16ToNSString(node_->GetTitle()) retain]);
    } else {
      NSString* newString =
        l10n_util::GetNSStringWithFixup(IDS_BOOKMARK_EDITOR_NEW_FOLDER_NAME);
      initialName_.reset([newString retain]);
    }
  }
  return self;
}

- (id)initWithParentWindow:(NSWindow*)window
                   profile:(Profile*)profile
                      node:(const BookmarkNode*)node {
  DCHECK(node);
  return [self initWithParentWindow:window
                            profile:profile
                               node:node
                             parent:nil
                           newIndex:0];
}

- (id)initWithParentWindow:(NSWindow*)window
                   profile:(Profile*)profile
                    parent:(const BookmarkNode*)parent
                  newIndex:(int)newIndex {
  DCHECK(parent);
  return [self initWithParentWindow:window
                            profile:profile
                               node:nil
                             parent:parent
                           newIndex:newIndex];
}

- (void)awakeFromNib {
  [nameField_ setStringValue:initialName_.get()];
  [[nameField_ cell] setUsesSingleLineMode:YES];

  [okButton_ setTitle:l10n_util::GetNSStringWithFixup(node_ ? IDS_SAVE :
                                                              IDS_ADD)];
}

- (void)runAsModalSheet {
  // Ping me when things change out from under us.
  observer_.reset(new BookmarkModelObserverForCocoa(
      BookmarkModelFactory::GetForBrowserContext(profile_), ^{
        [self cancel:nil];
      }));
  observer_->StartObservingNode(node_);
  [NSApp beginSheet:[self window]
     modalForWindow:parentWindow_
      modalDelegate:self
     didEndSelector:@selector(didEndSheet:returnCode:contextInfo:)
        contextInfo:nil];
}

- (IBAction)cancel:(id)sender {
  [NSApp endSheet:[self window]];
}

- (IBAction)ok:(id)sender {
  NSString* name = [nameField_ stringValue];
  if ([name length] == 0)
    name = l10n_util::GetNSStringWithFixup(IDS_BOOKMARK_EDITOR_NEW_FOLDER_NAME);
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile_);
  if (node_) {
    model->SetTitle(node_, base::SysNSStringToUTF16(name));
  } else {
    model->AddFolder(parent_,
                     newIndex_,
                     base::SysNSStringToUTF16(name));
  }
  [NSApp endSheet:[self window]];
}

- (void)didEndSheet:(NSWindow*)sheet
         returnCode:(int)returnCode
        contextInfo:(void*)contextInfo {
  [[self window] orderOut:self];
  observer_.reset(NULL);
  [self autorelease];
}

- (NSString*)folderName {
  return [nameField_ stringValue];
}

- (void)setFolderName:(NSString*)name {
  [nameField_ setStringValue:name];
}

- (NSButton*)okButton {
  return okButton_;
}

@end  // BookmarkNameFolderController

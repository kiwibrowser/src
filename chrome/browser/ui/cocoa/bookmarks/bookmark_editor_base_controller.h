// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_EDITOR_BASE_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_EDITOR_BASE_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/availability.h"
#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/bookmarks/bookmark_editor.h"
#include "components/bookmarks/browser/bookmark_expanded_state_tracker.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"

class BookmarkEditorBaseControllerBridge;
@class BookmarkTreeBrowserCell;

namespace bookmarks {
class BookmarkModel;
}

// A base controller class for bookmark creation and editing dialogs which
// present the current bookmark folder structure in a tree view.  Do not
// instantiate this controller directly -- use one of its derived classes.
// NOTE: If a derived class is intended to be dispatched via the
// BookmarkEditor::Show static function found in the accompanying
// implementation, that function will need to be update.
@interface BookmarkEditorBaseController
    : NSWindowController<NSOutlineViewDelegate, NSTouchBarDelegate> {
 @private
  IBOutlet NSButton* newFolderButton_;
  IBOutlet NSButton* okButton_;  // Used for unit testing only.
  IBOutlet NSTreeController* folderTreeController_;
  IBOutlet NSOutlineView* folderTreeView_;

  NSWindow* parentWindow_;  // weak
  Profile* profile_;  // weak
  const bookmarks::BookmarkNode* parentNode_;  // weak; owned by the model
  GURL url_;  // This and title_ are only used for new urls.
  base::string16 title_;
  BookmarkEditor::Configuration configuration_;
  NSString* initialName_;
  NSString* displayName_;  // Bound to a text field in the dialog.
  BOOL creatingNewFolders_;  // True while in createNewFolders.
  // An array of BookmarkFolderInfo where each item describes a folder in the
  // BookmarkNode structure.
  base::scoped_nsobject<NSArray> folderTreeArray_;
  // Bound to the table view giving a path to the current selections, of which
  // there should only ever be one.
  base::scoped_nsobject<NSArray> tableSelectionPaths_;
  // C++ bridge object that observes the BookmarkModel for me.
  std::unique_ptr<BookmarkEditorBaseControllerBridge> observer_;
}

@property(nonatomic, copy) NSString* initialName;
@property(nonatomic, copy) NSString* displayName;
@property(nonatomic, retain, readonly) NSArray* folderTreeArray;
@property(nonatomic, copy) NSArray* tableSelectionPaths;

// Designated initializer.  Derived classes should call through to this init.
// |url| and |title| are only used for BookmarkNode::Type::NEW_URL.
- (id)initWithParentWindow:(NSWindow*)parentWindow
                   nibName:(NSString*)nibName
                   profile:(Profile*)profile
                    parent:(const bookmarks::BookmarkNode*)parent
                       url:(const GURL&)url
                     title:(const base::string16&)title
             configuration:(BookmarkEditor::Configuration)configuration;

- (void)windowWillClose:(NSNotification*)notification;

// Override to customize the touch bar.
- (NSTouchBar*)makeTouchBar API_AVAILABLE(macos(10.12.2));

// Run the bookmark editor as a modal sheet.  Does not block.
- (void)runAsModalSheet;

// Create a new folder at the end of the selected parent folder, give it
// an untitled name, and put it into editing mode.
- (IBAction)newFolder:(id)sender;

// The cancel action will dismiss the dialog.  Derived classes which
// override cancel:, must call this after accessing any dialog-related
// data.
- (IBAction)cancel:(id)sender;

// The OK action will dismiss the dialog.  This action is bound
// to the OK button of a dialog which presents a tree view of a profile's
// folder hierarchy and allows the creation of new folders within that tree.
// When the OK button is pressed, this function will: 1) call the derived
// class's -[willCommit] function, 2) create any new folders created by
// the user while the dialog is presented, 3) call the derived class's
// -[didCommit] function, and then 4) dismiss the dialog.  At least one
// of -[willCommit] and -[didCommit] must be provided by the derived class
// and should return a NSNumber containing a BOOL or nil ('nil' means YES)
// indicating if the operation should be allowed to continue.
// Note: A derived class should not override the ok: action.
- (IBAction)ok:(id)sender;

// Methods for use by derived classes only.

// Determine and returns the rightmost selected/highlighted element (node)
// in the bookmark tree view if the tree view is showing, otherwise returns
// the original |parentNode_|.  If the tree view is showing but nothing is
// selected then the root node is returned.
- (const bookmarks::BookmarkNode*)selectedNode;

// Expands the set of BookmarkNodes in |nodes|.
- (void)expandNodes:(
    const bookmarks::BookmarkExpandedStateTracker::Nodes&)nodes;

// Returns the set of expanded BookmarkNodes.
- (bookmarks::BookmarkExpandedStateTracker::Nodes)getExpandedNodes;

// Select/highlight the given node within the browser tree view.  If the
// node is nil then select the bookmark bar node.  Exposed for unit test.
- (void)selectNodeInBrowser:(const bookmarks::BookmarkNode*)node;

// Notifications called when the BookmarkModel changes out from under me.
- (void)nodeRemoved:(const bookmarks::BookmarkNode*)node
         fromParent:(const bookmarks::BookmarkNode*)parent;
- (void)modelChangedPreserveSelection:(BOOL)preserve;

// Determines if the ok button should be enabled, can be overridden.
- (BOOL)okEnabled;

// Accessors
- (bookmarks::BookmarkModel*)bookmarkModel;
- (Profile*)profile;
- (const bookmarks::BookmarkNode*)parentNode;
- (const GURL&)url;
- (const base::string16&)title;

@end

// Describes the profile's bookmark folder structure: the folder name, the
// original BookmarkNode pointer (if the folder already exists), a BOOL
// indicating if the folder is new (meaning: created during this session
// but not yet committed to the bookmark structure), and an NSArray of
// child folder BookmarkFolderInfo's following this same structure.
@interface BookmarkFolderInfo : NSObject {
 @private
  NSString* folderName_;
  const bookmarks::BookmarkNode* folderNode_;  // weak
  NSMutableArray* children_;
  BOOL newFolder_;
}

@property(nonatomic, copy) NSString* folderName;
@property(nonatomic, assign) const bookmarks::BookmarkNode* folderNode;
@property(nonatomic, retain) NSMutableArray* children;
@property(nonatomic, assign) BOOL newFolder;

// Convenience creator for adding a new folder to the editor's bookmark
// structure.  This folder will be added to the bookmark model when the
// user accepts the dialog. |folderName| must be provided.
+ (id)bookmarkFolderInfoWithFolderName:(NSString*)folderName;

// Designated initializer.  |folderName| must be provided.  For folders which
// already exist in the bookmark model, |folderNode| and |children| (if any
// children are already attached to this folder) must be provided and
// |newFolder| should be NO.  For folders which the user has added during
// this session and which have not been committed yet, |newFolder| should be
// YES and |folderNode| and |children| should be NULL/nil.
- (id)initWithFolderName:(NSString*)folderName
              folderNode:(const bookmarks::BookmarkNode*)folderNode
                children:(NSMutableArray*)children
               newFolder:(BOOL)newFolder;

// Convenience creator used during construction of the editor's bookmark
// structure.  |folderName| and |folderNode| must be provided. |children|
// is optional.  Private: exposed here for unit testing purposes.
+ (id)bookmarkFolderInfoWithFolderName:(NSString*)folderName
                            folderNode:
                                (const bookmarks::BookmarkNode*)folderNode
                              children:(NSMutableArray*)children;

@end

@interface BookmarkEditorBaseController(TestingAPI)

@property(nonatomic, readonly) BOOL okButtonEnabled;

// Create any newly added folders.  New folders are nodes in folderTreeArray
// which are marked as being new (i.e. their kFolderTreeNewFolderKey
// dictionary item is YES).  This is called by -[ok:].
- (void)createNewFolders;

// Select the given bookmark node within the tree view.
- (void)selectTestNodeInBrowser:(const bookmarks::BookmarkNode*)node;

// Return the dictionary for the folder selected in the tree.
- (BookmarkFolderInfo*)selectedFolder;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_EDITOR_BASE_CONTROLLER_H_

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_editor_base_controller.h"

#include "base/auto_reset.h"
#include "base/containers/stack.h"
#include "base/logging.h"
#include "base/mac/availability.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_dialogs.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_all_tabs_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_editor_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_name_folder_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_tree_browser_cell.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/cocoa/touch_bar_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/strings/grit/ui_strings.h"

using bookmarks::BookmarkExpandedStateTracker;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace {

// Touch bar identifier.
NSString* const kBookmarkEditDialogTouchBarId = @"bookmark-edit-dialog";

// Touch bar item identifiers.
NSString* const kNewFolderTouchBarId = @"NEW-FOLDER";
NSString* const kCancelTouchBarId = @"CANCEL";
NSString* const kSaveTouchBarId = @"SAVE";

}  // end namespace

@interface BookmarkEditorBaseController ()

// Return the folder tree object for the given path.
- (BookmarkFolderInfo*)folderForIndexPath:(NSIndexPath*)path;

// (Re)build the folder tree from the BookmarkModel's current state.
- (void)buildFolderTree;

// Notifies the controller that the bookmark model has changed.
// |selection| specifies if the current selection should be
// maintained (usually YES).
- (void)modelChangedPreserveSelection:(BOOL)preserve;

// Notifies the controller that a node has been removed.
- (void)nodeRemoved:(const BookmarkNode*)node
         fromParent:(const BookmarkNode*)parent;

// Given a folder node, collect an array containing BookmarkFolderInfos
// describing its subchildren which are also folders.
- (NSMutableArray*)addChildFoldersFromNode:(const BookmarkNode*)node;

// Scan the folder tree stemming from the given tree folder and create
// any newly added folders.  Pass down info for the folder which was
// selected before we began creating folders.
- (void)createNewFoldersForFolder:(BookmarkFolderInfo*)treeFolder
               selectedFolderInfo:(BookmarkFolderInfo*)selectedFolderInfo;

// Scan the folder tree looking for the given bookmark node and return
// the selection path thereto.
- (NSIndexPath*)selectionPathForNode:(const BookmarkNode*)node;

// Implementation of getExpandedNodes. See description in header for details.
- (void)getExpandedNodes:(BookmarkExpandedStateTracker::Nodes*)nodes
                  folder:(BookmarkFolderInfo*)info
                    path:(std::vector<NSUInteger>*)path
                    root:(id)root;
@end

// static; implemented for each platform.  Update this function for new
// classes derived from BookmarkEditorBaseController.
void BookmarkEditor::Show(gfx::NativeWindow parent_window,
                          Profile* profile,
                          const EditDetails& details,
                          Configuration configuration) {
  if (chrome::ShowAllDialogsWithViewsToolkit()) {
    chrome::ShowBookmarkEditorViews(parent_window, profile, details,
                                    configuration);
    return;
  }

  if (details.type == EditDetails::EXISTING_NODE &&
      details.existing_node->is_folder()) {
    BookmarkNameFolderController* controller =
        [[BookmarkNameFolderController alloc]
            initWithParentWindow:parent_window
                         profile:profile
                            node:details.existing_node];
    [controller runAsModalSheet];
    return;
  }

  if (details.type == EditDetails::NEW_FOLDER && details.urls.empty()) {
    BookmarkNameFolderController* controller =
        [[BookmarkNameFolderController alloc]
             initWithParentWindow:parent_window
                          profile:profile
                           parent:details.parent_node
                         newIndex:details.index];
     [controller runAsModalSheet];
     return;
  }

  BookmarkEditorBaseController* controller = nil;
  if (details.type == EditDetails::NEW_FOLDER) {
    controller = [[BookmarkAllTabsController alloc]
                  initWithParentWindow:parent_window
                               profile:profile
                                parent:details.parent_node
                                   url:details.url
                                 title:details.title
                         configuration:configuration];
  } else {
    controller = [[BookmarkEditorController alloc]
                  initWithParentWindow:parent_window
                               profile:profile
                                parent:details.parent_node
                                  node:details.existing_node
                                   url:details.url
                                 title:details.title
                         configuration:configuration];
  }
  [controller runAsModalSheet];
}

// Adapter to tell BookmarkEditorBaseController when bookmarks change.
class BookmarkEditorBaseControllerBridge
    : public bookmarks::BookmarkModelObserver {
 public:
  BookmarkEditorBaseControllerBridge(BookmarkEditorBaseController* controller)
      : controller_(controller),
        importing_(false)
  { }

  // bookmarks::BookmarkModelObserver:
  void BookmarkModelLoaded(BookmarkModel* model, bool ids_reassigned) override {
    [controller_ modelChangedPreserveSelection:YES];
  }

  void BookmarkNodeMoved(BookmarkModel* model,
                         const BookmarkNode* old_parent,
                         int old_index,
                         const BookmarkNode* new_parent,
                         int new_index) override {
    if (!importing_ && new_parent->GetChild(new_index)->is_folder())
      [controller_ modelChangedPreserveSelection:YES];
  }

  void BookmarkNodeAdded(BookmarkModel* model,
                         const BookmarkNode* parent,
                         int index) override {
    if (!importing_ && parent->GetChild(index)->is_folder())
      [controller_ modelChangedPreserveSelection:YES];
  }

  void BookmarkNodeRemoved(BookmarkModel* model,
                           const BookmarkNode* parent,
                           int old_index,
                           const BookmarkNode* node,
                           const std::set<GURL>& removed_urls) override {
    [controller_ nodeRemoved:node fromParent:parent];
    if (node->is_folder())
      [controller_ modelChangedPreserveSelection:NO];
  }

  void BookmarkAllUserNodesRemoved(
      BookmarkModel* model,
      const std::set<GURL>& removed_urls) override {
    [controller_ modelChangedPreserveSelection:NO];
  }

  void BookmarkNodeChanged(BookmarkModel* model,
                           const BookmarkNode* node) override {
    if (!importing_ && node->is_folder())
      [controller_ modelChangedPreserveSelection:YES];
  }

  void BookmarkNodeChildrenReordered(BookmarkModel* model,
                                     const BookmarkNode* node) override {
    if (!importing_)
      [controller_ modelChangedPreserveSelection:YES];
  }

  void BookmarkNodeFaviconChanged(BookmarkModel* model,
                                  const BookmarkNode* node) override {
    // I care nothing for these 'favicons': I only show folders.
  }

  void ExtensiveBookmarkChangesBeginning(BookmarkModel* model) override {
    importing_ = true;
  }

  // Invoked after a batch import finishes.  This tells observers to update
  // themselves if they were waiting for the update to finish.
  void ExtensiveBookmarkChangesEnded(BookmarkModel* model) override {
    importing_ = false;
    [controller_ modelChangedPreserveSelection:YES];
  }

 private:
  BookmarkEditorBaseController* controller_;  // weak
  bool importing_;
};


#pragma mark -

@implementation BookmarkEditorBaseController

@synthesize initialName = initialName_;
@synthesize displayName = displayName_;

- (id)initWithParentWindow:(NSWindow*)parentWindow
                   nibName:(NSString*)nibName
                   profile:(Profile*)profile
                    parent:(const BookmarkNode*)parent
                       url:(const GURL&)url
                     title:(const base::string16&)title
             configuration:(BookmarkEditor::Configuration)configuration {
  NSString* nibpath = [base::mac::FrameworkBundle()
                        pathForResource:nibName
                                 ofType:@"nib"];
  if ((self = [super initWithWindowNibPath:nibpath owner:self])) {
    parentWindow_ = parentWindow;
    profile_ = profile;
    parentNode_ = parent;
    url_ = url;
    title_ = title;
    configuration_ = configuration;
    initialName_ = [@"" retain];
    observer_.reset(new BookmarkEditorBaseControllerBridge(self));
    [self bookmarkModel]->AddObserver(observer_.get());
  }
  return self;
}

- (void)dealloc {
  [self bookmarkModel]->RemoveObserver(observer_.get());
  [initialName_ release];
  [displayName_ release];
  [super dealloc];
}

- (void)awakeFromNib {
  [self setDisplayName:[self initialName]];

  if (configuration_ != BookmarkEditor::SHOW_TREE) {
    // Remember the tree view's height; we will shrink our frame by that much.
    NSRect frame = [[self window] frame];
    CGFloat browserHeight = [folderTreeView_ frame].size.height;
    frame.size.height -= browserHeight;
    frame.origin.y += browserHeight;
    // Remove the folder tree and "new folder" button.
    [folderTreeView_ removeFromSuperview];
    [newFolderButton_ removeFromSuperview];
    // Finally, commit the size change.
    [[self window] setFrame:frame display:YES];
  }

  // Build up a tree of the current folder configuration.
  [self buildFolderTree];
}

- (void)windowDidLoad {
  if (configuration_ == BookmarkEditor::SHOW_TREE) {
    [self selectNodeInBrowser:parentNode_];
  }
}

/* TODO(jrg):
// Implementing this informal protocol allows us to open the sheet
// somewhere other than at the top of the window.  NOTE: this means
// that I, the controller, am also the window's delegate.
- (NSRect)window:(NSWindow*)window willPositionSheet:(NSWindow*)sheet
        usingRect:(NSRect)rect {
  // adjust rect.origin.y to be the bottom of the toolbar
  return rect;
}
*/

// TODO(jrg): consider NSModalSession.
- (void)runAsModalSheet {
  // Lock down floating bar when in full-screen mode.  Don't animate
  // otherwise the pane will be misplaced.
  [[BrowserWindowController browserWindowControllerForWindow:parentWindow_]
      lockToolbarVisibilityForOwner:self
                      withAnimation:NO];
  [NSApp beginSheet:[self window]
     modalForWindow:parentWindow_
      modalDelegate:self
     didEndSelector:@selector(didEndSheet:returnCode:contextInfo:)
        contextInfo:nil];
}

// This constant has to match the name of the method after it.
NSString* const kOkEnabledName = @"okEnabled";
- (BOOL)okEnabled {
  return YES;
}

- (IBAction)ok:(id)sender {
  NSWindow* window = [self window];
  [window makeFirstResponder:window];
  // At least one of these two functions should be provided by derived classes.
  BOOL hasWillCommit = [self respondsToSelector:@selector(willCommit)];
  BOOL hasDidCommit = [self respondsToSelector:@selector(didCommit)];
  DCHECK(hasWillCommit || hasDidCommit);
  BOOL shouldContinue = YES;
  if (hasWillCommit) {
    NSNumber* hasWillContinue = [self performSelector:@selector(willCommit)];
    if (hasWillContinue && [hasWillContinue isKindOfClass:[NSNumber class]])
      shouldContinue = [hasWillContinue boolValue];
  }
  if (shouldContinue)
    [self createNewFolders];
  if (hasDidCommit) {
    NSNumber* hasDidContinue = [self performSelector:@selector(didCommit)];
    if (hasDidContinue && [hasDidContinue isKindOfClass:[NSNumber class]])
      shouldContinue = [hasDidContinue boolValue];
  }
  if (shouldContinue)
    [NSApp endSheet:window];
}

- (IBAction)cancel:(id)sender {
  [NSApp endSheet:[self window]];
}

- (void)didEndSheet:(NSWindow*)sheet
         returnCode:(int)returnCode
        contextInfo:(void*)contextInfo {
  [sheet close];
  [[BrowserWindowController browserWindowControllerForWindow:parentWindow_]
      releaseToolbarVisibilityForOwner:self
                         withAnimation:YES];
}

- (void)windowWillClose:(NSNotification*)notification {
  [self autorelease];
}

- (NSTouchBar*)makeTouchBar {
  if (!base::FeatureList::IsEnabled(features::kDialogTouchBar))
    return nil;

  base::scoped_nsobject<NSTouchBar> touchBar([[ui::NSTouchBar() alloc] init]);
  [touchBar setCustomizationIdentifier:ui::GetTouchBarId(
                                           kBookmarkEditDialogTouchBarId)];
  [touchBar setDelegate:self];

  NSArray* dialogItems = @[
    ui::GetTouchBarItemId(kBookmarkEditDialogTouchBarId, kNewFolderTouchBarId),
    ui::GetTouchBarItemId(kBookmarkEditDialogTouchBarId, kCancelTouchBarId),
    ui::GetTouchBarItemId(kBookmarkEditDialogTouchBarId, kSaveTouchBarId)
  ];

  [touchBar setDefaultItemIdentifiers:dialogItems];
  [touchBar setCustomizationAllowedItemIdentifiers:dialogItems];
  return touchBar.autorelease();
}

- (NSTouchBarItem*)touchBar:(NSTouchBar*)touchBar
      makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
    API_AVAILABLE(macos(10.12.2)) {
  NSButton* button = nil;
  if ([identifier hasSuffix:kNewFolderTouchBarId]) {
    button =
        [NSButton buttonWithTitle:l10n_util::GetNSString(
                                      IDS_BOOKMARK_EDITOR_NEW_FOLDER_BUTTON)
                           target:self
                           action:@selector(newFolder:)];
  } else if ([identifier hasSuffix:kCancelTouchBarId]) {
    button = [NSButton buttonWithTitle:l10n_util::GetNSString(IDS_APP_CANCEL)
                                target:self
                                action:@selector(cancel:)];
  } else if ([identifier hasSuffix:kSaveTouchBarId]) {
    button = ui::GetBlueTouchBarButton(l10n_util::GetNSString(IDS_SAVE), self,
                                       @selector(ok:));
  } else {
    return nil;
  }

  base::scoped_nsobject<NSCustomTouchBarItem> item(
      [[ui::NSCustomTouchBarItem() alloc] initWithIdentifier:identifier]);
  [item setView:button];
  return item.autorelease();
}

#pragma mark Folder Tree Management

- (BookmarkModel*)bookmarkModel {
  return BookmarkModelFactory::GetForBrowserContext(profile_);
}

- (Profile*)profile {
  return profile_;
}

- (const BookmarkNode*)parentNode {
  return parentNode_;
}

- (const GURL&)url {
  return url_;
}

- (const base::string16&)title{
  return title_;
}

- (BookmarkFolderInfo*)folderForIndexPath:(NSIndexPath*)indexPath {
  NSUInteger pathCount = [indexPath length];
  BookmarkFolderInfo* item = nil;
  NSArray* treeNode = [self folderTreeArray];
  for (NSUInteger i = 0; i < pathCount; ++i) {
    item = [treeNode objectAtIndex:[indexPath indexAtPosition:i]];
    treeNode = [item children];
  }
  return item;
}

- (NSIndexPath*)selectedIndexPath {
  NSIndexPath* selectedIndexPath = nil;
  NSArray* selections = [self tableSelectionPaths];
  if ([selections count]) {
    DCHECK([selections count] == 1);  // Should be exactly one selection.
    selectedIndexPath = [selections objectAtIndex:0];
  }
  return selectedIndexPath;
}

- (BookmarkFolderInfo*)selectedFolder {
  BookmarkFolderInfo* item = nil;
  NSIndexPath* selectedIndexPath = [self selectedIndexPath];
  if (selectedIndexPath) {
    item = [self folderForIndexPath:selectedIndexPath];
  }
  return item;
}

- (const BookmarkNode*)selectedNode {
  const BookmarkNode* selectedNode = NULL;
  // Determine a new parent node only if the browser is showing.
  if (configuration_ == BookmarkEditor::SHOW_TREE) {
    BookmarkFolderInfo* folderInfo = [self selectedFolder];
    if (folderInfo)
      selectedNode = [folderInfo folderNode];
  } else {
    // If the tree is not showing then we use the original parent.
    selectedNode = parentNode_;
  }
  return selectedNode;
}

- (void)expandNodes:(const BookmarkExpandedStateTracker::Nodes&)nodes {
  id treeControllerRoot = [folderTreeController_ arrangedObjects];
  for (BookmarkExpandedStateTracker::Nodes::const_iterator i = nodes.begin();
       i != nodes.end(); ++i) {
    NSIndexPath* path = [self selectionPathForNode:*i];
    id folderNode = [treeControllerRoot descendantNodeAtIndexPath:path];
    [folderTreeView_ expandItem:folderNode];
  }
}

- (BookmarkExpandedStateTracker::Nodes)getExpandedNodes {
  BookmarkExpandedStateTracker::Nodes nodes;
  std::vector<NSUInteger> path;
  NSArray* folderNodes = [self folderTreeArray];
  NSUInteger i = 0;
  for (BookmarkFolderInfo* folder in folderNodes) {
    path.push_back(i);
    [self getExpandedNodes:&nodes
                    folder:folder
                      path:&path
                      root:[folderTreeController_ arrangedObjects]];
    path.clear();
    ++i;
  }
  return nodes;
}

- (void)getExpandedNodes:(BookmarkExpandedStateTracker::Nodes*)nodes
                  folder:(BookmarkFolderInfo*)folder
                    path:(std::vector<NSUInteger>*)path
                    root:(id)root {
  NSIndexPath* indexPath = [NSIndexPath indexPathWithIndexes:&(path->front())
                                                      length:path->size()];
  id node = [root descendantNodeAtIndexPath:indexPath];
  if (![folderTreeView_ isItemExpanded:node])
    return;
  nodes->insert([folder folderNode]);
  NSArray* children = [folder children];
  NSUInteger i = 0;
  for (BookmarkFolderInfo* childFolder in children) {
    path->push_back(i);
    [self getExpandedNodes:nodes folder:childFolder path:path root:root];
    path->pop_back();
    ++i;
  }
}

- (NSArray*)folderTreeArray {
  return folderTreeArray_.get();
}

- (NSArray*)tableSelectionPaths {
  return tableSelectionPaths_.get();
}

- (void)setTableSelectionPath:(NSIndexPath*)tableSelectionPath {
  [self setTableSelectionPaths:[NSArray arrayWithObject:tableSelectionPath]];
}

- (void)setTableSelectionPaths:(NSArray*)tableSelectionPaths {
  tableSelectionPaths_.reset([tableSelectionPaths retain]);
}

- (void)selectNodeInBrowser:(const BookmarkNode*)node {
  DCHECK(configuration_ == BookmarkEditor::SHOW_TREE);
  NSIndexPath* selectionPath = [self selectionPathForNode:node];
  [self willChangeValueForKey:kOkEnabledName];
  [self setTableSelectionPath:selectionPath];
  [self didChangeValueForKey:kOkEnabledName];
}

- (NSIndexPath*)selectionPathForNode:(const BookmarkNode*)desiredNode {
  // Back up the parent chain for desiredNode, building up a stack
  // of ancestor nodes.  Then crawl down the folderTreeArray looking
  // for each ancestor in order while building up the selectionPath.
  base::stack<const BookmarkNode*> nodeStack;
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile_);
  const BookmarkNode* rootNode = model->root_node();
  const BookmarkNode* node = desiredNode;
  while (node != rootNode) {
    DCHECK(node);
    nodeStack.push(node);
    node = node->parent();
  }
  NSUInteger stackSize = nodeStack.size();

  NSIndexPath* path = nil;
  NSArray* folders = [self folderTreeArray];
  while (!nodeStack.empty()) {
    node = nodeStack.top();
    nodeStack.pop();
    // Find node in the current folders array.
    NSUInteger i = 0;
    for (BookmarkFolderInfo *folderInfo in folders) {
      const BookmarkNode* testNode = [folderInfo folderNode];
      if (testNode == node) {
        path = path ? [path indexPathByAddingIndex:i] :
        [NSIndexPath indexPathWithIndex:i];
        folders = [folderInfo children];
        break;
      }
      ++i;
    }
  }
  DCHECK([path length] == stackSize);
  return path;
}

- (NSMutableArray*)addChildFoldersFromNode:(const BookmarkNode*)node {
  bookmarks::ManagedBookmarkService* managed =
      ManagedBookmarkServiceFactory::GetForProfile(profile_);
  NSMutableArray* childFolders = nil;
  int childCount = node->child_count();
  for (int i = 0; i < childCount; ++i) {
    const BookmarkNode* childNode = node->GetChild(i);
    if (childNode->is_folder() && childNode->IsVisible() &&
        managed->CanBeEditedByUser(childNode)) {
      NSString* childName = base::SysUTF16ToNSString(childNode->GetTitle());
      NSMutableArray* children = [self addChildFoldersFromNode:childNode];
      BookmarkFolderInfo* folderInfo =
          [BookmarkFolderInfo bookmarkFolderInfoWithFolderName:childName
                                                    folderNode:childNode
                                                      children:children];
      if (!childFolders)
        childFolders = [NSMutableArray arrayWithObject:folderInfo];
      else
        [childFolders addObject:folderInfo];
    }
  }
  return childFolders;
}

- (void)buildFolderTree {
  // Build up a tree of the current folder configuration.
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile_);
  const BookmarkNode* rootNode = model->root_node();
  NSMutableArray* baseArray = [self addChildFoldersFromNode:rootNode];
  DCHECK(baseArray);
  [self willChangeValueForKey:@"folderTreeArray"];
  folderTreeArray_.reset([baseArray retain]);
  [self didChangeValueForKey:@"folderTreeArray"];
}

- (void)modelChangedPreserveSelection:(BOOL)preserve {
  if (creatingNewFolders_)
    return;
  const BookmarkNode* selectedNode = [self selectedNode];
  [self buildFolderTree];
  if (preserve &&
      selectedNode &&
      configuration_ == BookmarkEditor::SHOW_TREE)
    [self selectNodeInBrowser:selectedNode];
}

- (void)nodeRemoved:(const BookmarkNode*)node
         fromParent:(const BookmarkNode*)parent {
  if (node->is_folder()) {
    if (parentNode_ == node || parentNode_->HasAncestor(node)) {
      parentNode_ = [self bookmarkModel]->bookmark_bar_node();
      if (configuration_ != BookmarkEditor::SHOW_TREE) {
        // The user can't select a different folder, so just close up shop.
        [self cancel:self];
        return;
      }
    }

    if (configuration_ == BookmarkEditor::SHOW_TREE) {
      // For safety's sake, in case deleted node was an ancestor of selection,
      // go back to a known safe place.
      [self selectNodeInBrowser:parentNode_];
    }
  }
}

#pragma mark New Folder Handler

- (void)createNewFoldersForFolder:(BookmarkFolderInfo*)folderInfo
               selectedFolderInfo:(BookmarkFolderInfo*)selectedFolderInfo {
  NSArray* subfolders = [folderInfo children];
  const BookmarkNode* parentNode = [folderInfo folderNode];
  DCHECK(parentNode);
  NSUInteger i = 0;
  for (BookmarkFolderInfo* subFolderInfo in subfolders) {
    if ([subFolderInfo newFolder]) {
      BookmarkModel* model = [self bookmarkModel];
      const BookmarkNode* newFolder =
          model->AddFolder(parentNode, i,
              base::SysNSStringToUTF16([subFolderInfo folderName]));
      // Update our dictionary with the actual folder node just created.
      [subFolderInfo setFolderNode:newFolder];
      [subFolderInfo setNewFolder:NO];
    }
    [self createNewFoldersForFolder:subFolderInfo
                 selectedFolderInfo:selectedFolderInfo];
    ++i;
  }
}

- (IBAction)newFolder:(id)sender {
  // Create a new folder off of the selected folder node.
  BookmarkFolderInfo* parentInfo = [self selectedFolder];
  if (parentInfo) {
    NSIndexPath* selection = [self selectedIndexPath];
    NSString* newFolderName =
        l10n_util::GetNSStringWithFixup(IDS_BOOKMARK_EDITOR_NEW_FOLDER_NAME);
    BookmarkFolderInfo* folderInfo =
        [BookmarkFolderInfo bookmarkFolderInfoWithFolderName:newFolderName];
    [self willChangeValueForKey:@"folderTreeArray"];
    NSMutableArray* children = [parentInfo children];
    if (children) {
      [children addObject:folderInfo];
    } else {
      children = [NSMutableArray arrayWithObject:folderInfo];
      [parentInfo setChildren:children];
    }
    [self didChangeValueForKey:@"folderTreeArray"];

    // Expose the parent folder children.
    [folderTreeView_ expandItem:parentInfo];

    // Select the new folder node and put the folder name into edit mode.
    selection = [selection indexPathByAddingIndex:[children count] - 1];
    [self setTableSelectionPath:selection];
    NSInteger row = [folderTreeView_ selectedRow];
    DCHECK(row >= 0);

    // Put the cell into single-line mode before putting it into edit mode.
    NSCell* folderCell = [folderTreeView_ preparedCellAtColumn:0 row:row];
    [folderCell setUsesSingleLineMode:YES];

    [folderTreeView_ editColumn:0 row:row withEvent:nil select:YES];
  }
}

- (void)createNewFolders {
  base::AutoReset<BOOL> creatingNewFoldersSetter(&creatingNewFolders_, YES);
  // Scan the tree looking for nodes marked 'newFolder' and create those nodes.
  NSArray* folderTreeArray = [self folderTreeArray];
  for (BookmarkFolderInfo *folderInfo in folderTreeArray) {
    [self createNewFoldersForFolder:folderInfo
                 selectedFolderInfo:[self selectedFolder]];
  }
}

#pragma mark For Unit Test Use Only

- (BOOL)okButtonEnabled {
  return [okButton_ isEnabled];
}

- (void)selectTestNodeInBrowser:(const BookmarkNode*)node {
  [self selectNodeInBrowser:node];
}

- (BOOL)outlineView:(NSOutlineView*)outlineView
    shouldEditTableColumn:(NSTableColumn*)tableColumn
                     item:(id)item {
  BookmarkFolderInfo* info =
      base::mac::ObjCCast<BookmarkFolderInfo>([item representedObject]);
  return info.newFolder;
}

@end  // BookmarkEditorBaseController

@implementation BookmarkFolderInfo

@synthesize folderName = folderName_;
@synthesize folderNode = folderNode_;
@synthesize children = children_;
@synthesize newFolder = newFolder_;

+ (id)bookmarkFolderInfoWithFolderName:(NSString*)folderName
                            folderNode:(const BookmarkNode*)folderNode
                              children:(NSMutableArray*)children {
  return [[[BookmarkFolderInfo alloc] initWithFolderName:folderName
                                              folderNode:folderNode
                                                children:children
                                               newFolder:NO]
          autorelease];
}

+ (id)bookmarkFolderInfoWithFolderName:(NSString*)folderName {
  return [[[BookmarkFolderInfo alloc] initWithFolderName:folderName
                                              folderNode:NULL
                                                children:nil
                                               newFolder:YES]
          autorelease];
}

- (id)initWithFolderName:(NSString*)folderName
              folderNode:(const BookmarkNode*)folderNode
                children:(NSMutableArray*)children
               newFolder:(BOOL)newFolder {
  if ((self = [super init])) {
    // A folderName is always required, and if newFolder is NO then there
    // should be a folderNode.  Children is optional.
    DCHECK(folderName && (newFolder || folderNode));
    if (folderName && (newFolder || folderNode)) {
      folderName_ = [folderName copy];
      folderNode_ = folderNode;
      children_ = [children retain];
      newFolder_ = newFolder;
    } else {
      NOTREACHED();  // Invalid init.
      [self release];
      self = nil;
    }
  }
  return self;
}

- (id)init {
  NOTREACHED();  // Should never be called.
  return [self initWithFolderName:nil folderNode:nil children:nil newFolder:NO];
}

- (void)dealloc {
  [folderName_ release];
  [children_ release];
  [super dealloc];
}

// Implementing isEqual: allows the NSTreeController to preserve the selection
// and open/shut state of outline items when the data changes.
- (BOOL)isEqual:(id)other {
  return [other isKindOfClass:[BookmarkFolderInfo class]] &&
      folderNode_ == [(BookmarkFolderInfo*)other folderNode];
}
@end

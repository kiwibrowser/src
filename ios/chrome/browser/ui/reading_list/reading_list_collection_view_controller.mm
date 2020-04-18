// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_controller.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/alert_coordinator/action_sheet_coordinator.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/list_model/list_item+Controller.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_item_accessibility_delegate.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_data_sink.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_data_source.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_empty_collection_background.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_toolbar.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/AppBar/src/MaterialAppBar.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierUnread = kSectionIdentifierEnumZero,
  SectionIdentifierRead,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeHeader = kItemTypeEnumZero,
  ItemTypeItem,
};

// Typedef for a block taking a CollectionViewItem as parameter and returning
// nothing.
typedef void (^EntryUpdater)(CollectionViewItem* item);
}

@interface ReadingListCollectionViewController ()<
    ReadingListCollectionViewItemAccessibilityDelegate,
    ReadingListDataSink,
    UIGestureRecognizerDelegate> {
  // Toolbar with the actions.
  ReadingListToolbar* _toolbar;
  // Action sheet presenting the subactions of the toolbar.
  AlertCoordinator* _actionSheet;
  UIView* _emptyCollectionBackground;

  // Whether the data source has pending modifications.
  BOOL _dataSourceHasBeenModified;
}

// Whether the data source modifications should be taken into account.
@property(nonatomic, assign) BOOL shouldMonitorDataSource;

// Handles "Done" button touches.
- (void)donePressed;
// Loads all the items in all sections.
- (void)loadItems;
// Fills section |sectionIdentifier| with the items from |array|.
- (void)loadItemsFromArray:(NSArray<CollectionViewItem*>*)array
                 toSection:(SectionIdentifier)sectionIdentifier;
// Reloads the data if a change occurred during editing
- (void)applyPendingUpdates;
// Returns whether there are elements in the section identified by
// |sectionIdentifier|.
- (BOOL)hasItemInSection:(SectionIdentifier)sectionIdentifier;
// Adds an empty background if needed.
- (void)collectionIsEmpty;
// Handles a long press.
- (void)handleLongPress:(UILongPressGestureRecognizer*)gestureRecognizer;
// Updates the toolbar state according to the selected items.
- (void)updateToolbarState;
// Displays an action sheet to let the user choose to mark all the elements as
// read or as unread. Used when nothing is selected.
- (void)markAllItemsAs;
// Displays an action sheet to let the user choose to mark all the selected
// elements as read or as unread. Used if read and unread elements are selected.
- (void)markMixedItemsAs;
// Marks all items as read.
- (void)markAllRead;
// Marks all items as unread.
- (void)markAllUnread;
// Marks the items at |indexPaths| as read.
- (void)markItemsReadAtIndexPath:(NSArray*)indexPaths;
// Marks the items at |indexPaths| as unread.
- (void)markItemsUnreadAtIndexPath:(NSArray*)indexPaths;
// Deletes all the read items.
- (void)deleteAllReadItems;
// Deletes all the items at |indexPath|.
- (void)deleteItemsAtIndexPaths:(NSArray*)indexPaths;
// Initializes |_actionSheet| with |self| as base view controller, and the
// toolbar's mark button as anchor point.
- (void)initializeActionSheet;
// Exits the editing mode and update the toolbar state with animation.
- (void)exitEditingModeAnimated:(BOOL)animated;
// Applies |updater| to the URL of every cell in the section |identifier|. The
// updates are done in reverse order of the cells in the section to keep the
// order. The monitoring of the data source updates are suspended during this
// time.
- (void)updateItemsInSectionIdentifier:(SectionIdentifier)identifier
                     usingEntryUpdater:(EntryUpdater)updater;
// Applies |updater| to the URL of every element in |indexPaths|. The updates
// are done in reverse order |indexPaths| to keep the order. The monitoring of
// the data source updates are suspended during this time.
- (void)updateIndexPaths:(NSArray<NSIndexPath*>*)indexPaths
       usingEntryUpdater:(EntryUpdater)updater;
// Move all the items from |sourceSectionIdentifier| to
// |destinationSectionIdentifier| and removes the empty section from the
// collection.
- (void)moveItemsFromSection:(SectionIdentifier)sourceSectionIdentifier
                   toSection:(SectionIdentifier)destinationSectionIdentifier;
// Move the currently selected elements to |sectionIdentifier| and removes the
// empty sections.
- (void)moveSelectedItems:(NSArray*)sortedIndexPaths
                toSection:(SectionIdentifier)sectionIdentifier;
// Makes sure |sectionIdentifier| exists with the correct header.
// Returns the index of the new section in the collection view; NSIntegerMax if
// no section has been created.
- (NSInteger)initializeSection:(SectionIdentifier)sectionIdentifier;
// Returns the header for the |sectionIdentifier|.
- (CollectionViewTextItem*)headerForSection:
    (SectionIdentifier)sectionIdentifier;
// Removes the empty sections from the collection and the model.
- (void)removeEmptySections;

@end

@implementation ReadingListCollectionViewController

@synthesize audience = _audience;
@synthesize delegate = _delegate;
@synthesize dataSource = _dataSource;

@synthesize shouldMonitorDataSource = _shouldMonitorDataSource;

+ (NSString*)accessibilityIdentifier {
  return @"ReadingListCollectionView";
}

#pragma mark lifecycle

- (instancetype)initWithDataSource:(id<ReadingListDataSource>)dataSource
                           toolbar:(ReadingListToolbar*)toolbar {
  UICollectionViewLayout* layout = [[MDCCollectionViewFlowLayout alloc] init];
  self =
      [super initWithLayout:layout style:CollectionViewControllerStyleAppBar];
  if (self) {
    _toolbar = toolbar;

    _dataSource = dataSource;
    _emptyCollectionBackground =
        [[ReadingListEmptyCollectionBackground alloc] init];

    _shouldMonitorDataSource = YES;
    _dataSourceHasBeenModified = NO;

    _dataSource.dataSink = self;
  }
  return self;
}

#pragma mark - properties

- (void)setToolbarState:(ReadingListToolbarState)toolbarState {
  [_toolbar setState:toolbarState];
}

- (void)setAudience:(id<ReadingListCollectionViewControllerAudience>)audience {
  _audience = audience;
  if (self.dataSource.ready) {
    [audience readingListHasItems:self.dataSource.hasElements];
  }
}

#pragma mark - UIViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  self.title = l10n_util::GetNSString(IDS_IOS_TOOLS_MENU_READING_LIST);
  self.collectionView.accessibilityIdentifier =
      [ReadingListCollectionViewController accessibilityIdentifier];
  // Add "Done" button.
  UIBarButtonItem* doneItem = [[UIBarButtonItem alloc]
      initWithTitle:l10n_util::GetNSString(IDS_IOS_READING_LIST_DONE_BUTTON)
              style:UIBarButtonItemStylePlain
             target:self
             action:@selector(donePressed)];
  doneItem.accessibilityIdentifier = @"Done";
  self.navigationItem.rightBarButtonItem = doneItem;

  // Customize collection view settings.
  self.styler.cellStyle = MDCCollectionViewCellStyleCard;
  self.styler.separatorInset = UIEdgeInsetsMake(0, 16, 0, 16);

  UILongPressGestureRecognizer* longPressRecognizer =
      [[UILongPressGestureRecognizer alloc]
          initWithTarget:self
                  action:@selector(handleLongPress:)];
  longPressRecognizer.delegate = self;
  [self.collectionView addGestureRecognizer:longPressRecognizer];
}

- (void)viewDidLayoutSubviews {
  [super viewDidLayoutSubviews];
  [_toolbar updateHeight];
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  [super collectionView:collectionView didSelectItemAtIndexPath:indexPath];

  if (self.editor.editing) {
    [self updateToolbarState];
  } else {
    [self.delegate
        readingListCollectionViewController:self
                                   openItem:[self.collectionViewModel
                                                itemAtIndexPath:indexPath]];
  }
}

- (void)collectionView:(UICollectionView*)collectionView
    didDeselectItemAtIndexPath:(NSIndexPath*)indexPath {
  [super collectionView:collectionView didDeselectItemAtIndexPath:indexPath];
  if (self.editor.editing) {
    // When deselecting an item, if we are editing, we want to update the
    // toolbar based on the selected items.
    [self updateToolbarState];
  }
}

#pragma mark - MDCCollectionViewController

- (void)updateFooterInfoBarIfNecessary {
  // No-op. This prevents the default infobar from showing.
  // TODO(crbug.com/653547): Remove this once the MDC adds an option for
  // preventing the infobar from showing.
}

#pragma mark - MDCCollectionViewStylingDelegate

- (CGFloat)collectionView:(UICollectionView*)collectionView
    cellHeightAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger type = [self.collectionViewModel itemTypeForIndexPath:indexPath];
  if (type == ItemTypeItem)
    return MDCCellDefaultTwoLineHeight;
  else
    return MDCCellDefaultOneLineHeight;
}

#pragma mark - MDCCollectionViewEditingDelegate

- (BOOL)collectionViewAllowsEditing:(nonnull UICollectionView*)collectionView {
  return YES;
}

#pragma mark - ReadingListDataSink

- (void)dataSourceReady:(id<ReadingListDataSource>)dataSource {
  [self loadModel];
  if ([self isViewLoaded]) {
    [self.collectionView reloadData];
  }
}

- (void)dataSourceChanged {
  // If we are editing and monitoring the model updates, set a flag to reload
  // the data at the end of the editing.
  if (self.editor.editing) {
    _dataSourceHasBeenModified = YES;
  } else {
    [self reloadData];
  }
}

- (NSArray<CollectionViewItem*>*)readItems {
  if (![self.collectionViewModel
          hasSectionForSectionIdentifier:SectionIdentifierRead]) {
    return nil;
  }
  return [self.collectionViewModel
      itemsInSectionWithIdentifier:SectionIdentifierRead];
}

- (NSArray<CollectionViewItem*>*)unreadItems {
  if (![self.collectionViewModel
          hasSectionForSectionIdentifier:SectionIdentifierUnread]) {
    return nil;
  }
  return [self.collectionViewModel
      itemsInSectionWithIdentifier:SectionIdentifierUnread];
}

- (void)itemHasChangedAfterDelay:(CollectionViewItem*)item {
  if ([self.collectionViewModel hasItem:item]) {
    [self reconfigureCellsForItems:@[ item ]];
  }
}

- (void)itemsHaveChanged:(NSArray<CollectionViewItem*>*)items {
  [self reconfigureCellsForItems:items];
}

#pragma mark - Public methods

- (void)reloadData {
  [self loadModel];
  if ([self isViewLoaded]) {
    [self.collectionView reloadData];
  }
}

- (void)willBeDismissed {
  [self.dataSource dataSinkWillBeDismissed];
  [_actionSheet stop];
}

#pragma mark - ReadingListCollectionViewItemAccessibilityDelegate

- (BOOL)isEntryRead:(CollectionViewItem*)entry {
  return [self.dataSource isEntryRead:entry];
}

- (void)deleteEntry:(CollectionViewItem*)entry {
  if ([self.collectionViewModel hasItem:entry]) {
    [self deleteItemsAtIndexPaths:@[ [self.collectionViewModel
                                      indexPathForItem:entry] ]];
  }
}

- (void)openEntryInNewTab:(CollectionViewItem*)entry {
  [self.delegate readingListCollectionViewController:self
                                    openItemInNewTab:entry
                                           incognito:NO];
}

- (void)openEntryInNewIncognitoTab:(CollectionViewItem*)entry {
  [self.delegate readingListCollectionViewController:self
                                    openItemInNewTab:entry
                                           incognito:YES];
}

- (void)openEntryOffline:(CollectionViewItem*)entry {
  [self.delegate readingListCollectionViewController:self
                             openItemOfflineInNewTab:entry];
}

- (void)markEntryRead:(CollectionViewItem*)entry {
  if ([self.collectionViewModel hasItem:entry
                inSectionWithIdentifier:SectionIdentifierUnread]) {
    [self markItemsReadAtIndexPath:@[ [self.collectionViewModel
                                       indexPathForItem:entry] ]];
  }
}

- (void)markEntryUnread:(CollectionViewItem*)entry {
  if ([self.collectionViewModel hasItem:entry
                inSectionWithIdentifier:SectionIdentifierRead]) {
    [self markItemsUnreadAtIndexPath:@[ [self.collectionViewModel
                                         indexPathForItem:entry] ]];
  }
}

#pragma mark - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
       shouldReceiveTouch:(UITouch*)touch {
  // Prevent the context menu to be displayed if the long press is done on the
  // App Bar.
  CGPoint location = [touch locationInView:self.appBar.navigationBar];
  return !CGRectContainsPoint(self.appBar.navigationBar.frame, location);
}

#pragma mark - Private methods

- (void)donePressed {
  if ([self.editor isEditing]) {
    [self exitEditingModeAnimated:NO];
  }
  [self dismiss];
}

- (void)dismiss {
  [self.delegate dismissReadingListCollectionViewController:self];
}

- (void)loadModel {
  [super loadModel];
  _dataSourceHasBeenModified = NO;

  if (!self.dataSource.hasElements) {
    [self collectionIsEmpty];
  } else {
    self.collectionView.alwaysBounceVertical = YES;
    [self loadItems];
    self.collectionView.backgroundView = nil;
    [self.audience readingListHasItems:YES];
  }
}

- (void)loadItemsFromArray:(NSArray<CollectionViewItem*>*)items
                 toSection:(SectionIdentifier)sectionIdentifier {
  if (items.count == 0) {
    return;
  }
  CollectionViewModel* model = self.collectionViewModel;
  [model addSectionWithIdentifier:sectionIdentifier];
  [model setHeader:[self headerForSection:sectionIdentifier]
      forSectionWithIdentifier:sectionIdentifier];
  for (CollectionViewItem* item in items) {
    item.type = ItemTypeItem;
    [self.dataSource fetchFaviconForItem:item];
    [model addItem:item toSectionWithIdentifier:sectionIdentifier];
  }
}

- (void)loadItems {
  NSMutableArray<CollectionViewItem*>* readArray = [NSMutableArray array];
  NSMutableArray<CollectionViewItem*>* unreadArray = [NSMutableArray array];
  [self.dataSource fillReadItems:readArray
                     unreadItems:unreadArray
                    withDelegate:self];
  [self loadItemsFromArray:unreadArray toSection:SectionIdentifierUnread];
  [self loadItemsFromArray:readArray toSection:SectionIdentifierRead];

  BOOL hasRead = readArray.count > 0;
  [_toolbar setHasReadItem:hasRead];
}

- (void)applyPendingUpdates {
  if (_dataSourceHasBeenModified) {
    [self reloadData];
  }
}

- (BOOL)hasItemInSection:(SectionIdentifier)sectionIdentifier {
  if (![self.collectionViewModel
          hasSectionForSectionIdentifier:sectionIdentifier]) {
    // No section.
    return NO;
  }

  NSInteger section =
      [self.collectionViewModel sectionForSectionIdentifier:sectionIdentifier];
  NSInteger numberOfItems =
      [self.collectionViewModel numberOfItemsInSection:section];

  return numberOfItems > 0;
}

- (void)collectionIsEmpty {
  if (self.collectionView.backgroundView) {
    return;
  }
  // The collection is empty, add background.
  self.collectionView.alwaysBounceVertical = NO;
  self.collectionView.backgroundView = _emptyCollectionBackground;
  [self.audience readingListHasItems:NO];
}

- (void)handleLongPress:(UILongPressGestureRecognizer*)gestureRecognizer {
  if (self.editor.editing ||
      gestureRecognizer.state != UIGestureRecognizerStateBegan) {
    return;
  }

  CGPoint touchLocation =
      [gestureRecognizer locationOfTouch:0 inView:self.collectionView];
  NSIndexPath* touchedItemIndexPath =
      [self.collectionView indexPathForItemAtPoint:touchLocation];
  if (!touchedItemIndexPath ||
      ![self.collectionViewModel hasItemAtIndexPath:touchedItemIndexPath]) {
    // Make sure there is an item at this position.
    return;
  }
  CollectionViewItem* touchedItem =
      [self.collectionViewModel itemAtIndexPath:touchedItemIndexPath];

  if (touchedItem.type != ItemTypeItem) {
    // Do not trigger context menu on headers.
    return;
  }

  [self.delegate readingListCollectionViewController:self
                           displayContextMenuForItem:touchedItem
                                             atPoint:touchLocation];
}

#pragma mark - ReadingListToolbarDelegate

- (void)markPressed {
  if (![self.editor isEditing]) {
    return;
  }
  switch ([_toolbar state]) {
    case NoneSelected:
      [self markAllItemsAs];
      break;
    case OnlyUnreadSelected:
      [self markItemsReadAtIndexPath:self.collectionView
                                         .indexPathsForSelectedItems];
      break;
    case OnlyReadSelected:
      [self markItemsUnreadAtIndexPath:self.collectionView
                                           .indexPathsForSelectedItems];
      break;
    case MixedItemsSelected:
      [self markMixedItemsAs];
      break;
  }
}

- (void)deletePressed {
  if (![self.editor isEditing]) {
    return;
  }
  if ([_toolbar state] == NoneSelected) {
    [self deleteAllReadItems];
  } else {
    [self
        deleteItemsAtIndexPaths:self.collectionView.indexPathsForSelectedItems];
  }
}
- (void)enterEditingModePressed {
  if ([self.editor isEditing]) {
    return;
  }
  self.toolbarState = NoneSelected;
  [self.editor setEditing:YES animated:YES];
  [_toolbar setEditing:YES];
}

- (void)exitEditingModePressed {
  if (![self.editor isEditing]) {
    return;
  }
  [self exitEditingModeAnimated:YES];
}

#pragma mark - Private methods - Toolbar

- (void)updateToolbarState {
  BOOL readSelected = NO;
  BOOL unreadSelected = NO;

  if ([self.collectionView.indexPathsForSelectedItems count] == 0) {
    // No entry selected.
    self.toolbarState = NoneSelected;
    return;
  }

  // Sections for section identifiers.
  NSInteger sectionRead = NSNotFound;
  NSInteger sectionUnread = NSNotFound;

  if ([self hasItemInSection:SectionIdentifierRead]) {
    sectionRead = [self.collectionViewModel
        sectionForSectionIdentifier:SectionIdentifierRead];
  }
  if ([self hasItemInSection:SectionIdentifierUnread]) {
    sectionUnread = [self.collectionViewModel
        sectionForSectionIdentifier:SectionIdentifierUnread];
  }

  // Check selected sections.
  for (NSIndexPath* index in self.collectionView.indexPathsForSelectedItems) {
    if (index.section == sectionRead) {
      readSelected = YES;
    } else if (index.section == sectionUnread) {
      unreadSelected = YES;
    }
  }

  // Update toolbar state.
  if (readSelected) {
    if (unreadSelected) {
      // Read and Unread selected.
      self.toolbarState = MixedItemsSelected;
    } else {
      // Read selected.
      self.toolbarState = OnlyReadSelected;
    }
  } else if (unreadSelected) {
    // Unread selected.
    self.toolbarState = OnlyUnreadSelected;
  }
}

- (void)markAllItemsAs {
  [self initializeActionSheet];
  __weak ReadingListCollectionViewController* weakSelf = self;
  [_actionSheet addItemWithTitle:l10n_util::GetNSStringWithFixup(
                                     IDS_IOS_READING_LIST_MARK_ALL_READ_ACTION)
                          action:^{
                            [weakSelf markAllRead];
                          }
                           style:UIAlertActionStyleDefault];
  [_actionSheet
      addItemWithTitle:l10n_util::GetNSStringWithFixup(
                           IDS_IOS_READING_LIST_MARK_ALL_UNREAD_ACTION)
                action:^{
                  [weakSelf markAllUnread];
                }
                 style:UIAlertActionStyleDefault];
  [_actionSheet start];
}

- (void)markMixedItemsAs {
  [self initializeActionSheet];
  __weak ReadingListCollectionViewController* weakSelf = self;
  [_actionSheet
      addItemWithTitle:l10n_util::GetNSStringWithFixup(
                           IDS_IOS_READING_LIST_MARK_READ_BUTTON)
                action:^{
                  [weakSelf
                      markItemsReadAtIndexPath:weakSelf.collectionView
                                                   .indexPathsForSelectedItems];
                }
                 style:UIAlertActionStyleDefault];
  [_actionSheet
      addItemWithTitle:l10n_util::GetNSStringWithFixup(
                           IDS_IOS_READING_LIST_MARK_UNREAD_BUTTON)
                action:^{
                  [weakSelf
                      markItemsUnreadAtIndexPath:
                          weakSelf.collectionView.indexPathsForSelectedItems];
                }
                 style:UIAlertActionStyleDefault];
  [_actionSheet start];
}

- (void)initializeActionSheet {
  _actionSheet = [_toolbar actionSheetForMarkWithBaseViewController:self];

  [_actionSheet addItemWithTitle:l10n_util::GetNSStringWithFixup(IDS_CANCEL)
                          action:nil
                           style:UIAlertActionStyleCancel];
}

- (void)markAllRead {
  if (![self.editor isEditing]) {
    return;
  }
  if (![self hasItemInSection:SectionIdentifierUnread]) {
    [self exitEditingModeAnimated:YES];
    return;
  }

  [self updateItemsInSectionIdentifier:SectionIdentifierUnread
                     usingEntryUpdater:^(CollectionViewItem* item) {
                       [self.dataSource setReadStatus:YES forItem:item];
                     }];

  [self exitEditingModeAnimated:YES];
  [self moveItemsFromSection:SectionIdentifierUnread
                   toSection:SectionIdentifierRead];
}

- (void)markAllUnread {
  if (![self hasItemInSection:SectionIdentifierRead]) {
    [self exitEditingModeAnimated:YES];
    return;
  }

  [self updateItemsInSectionIdentifier:SectionIdentifierRead
                     usingEntryUpdater:^(CollectionViewItem* item) {
                       [self.dataSource setReadStatus:NO forItem:item];
                     }];

  [self exitEditingModeAnimated:YES];
  [self moveItemsFromSection:SectionIdentifierRead
                   toSection:SectionIdentifierUnread];
}

- (void)markItemsReadAtIndexPath:(NSArray*)indexPaths {
  base::RecordAction(base::UserMetricsAction("MobileReadingListMarkRead"));
  NSArray* sortedIndexPaths =
      [indexPaths sortedArrayUsingSelector:@selector(compare:)];
  [self updateIndexPaths:sortedIndexPaths
       usingEntryUpdater:^(CollectionViewItem* item) {
         [self.dataSource setReadStatus:YES forItem:item];
       }];

  [self exitEditingModeAnimated:YES];
  [self moveSelectedItems:sortedIndexPaths toSection:SectionIdentifierRead];
}

- (void)markItemsUnreadAtIndexPath:(NSArray*)indexPaths {
  base::RecordAction(base::UserMetricsAction("MobileReadingListMarkUnread"));
  NSArray* sortedIndexPaths =
      [indexPaths sortedArrayUsingSelector:@selector(compare:)];
  [self updateIndexPaths:sortedIndexPaths
       usingEntryUpdater:^(CollectionViewItem* item) {
         [self.dataSource setReadStatus:NO forItem:item];
       }];

  [self exitEditingModeAnimated:YES];
  [self moveSelectedItems:sortedIndexPaths toSection:SectionIdentifierUnread];
}

- (void)deleteAllReadItems {
  base::RecordAction(base::UserMetricsAction("MobileReadingListDeleteRead"));
  if (![self hasItemInSection:SectionIdentifierRead]) {
    [self exitEditingModeAnimated:YES];
    return;
  }

  [self updateItemsInSectionIdentifier:SectionIdentifierRead
                     usingEntryUpdater:^(CollectionViewItem* item) {
                       [self.dataSource removeEntryFromItem:item];
                     }];

  [self exitEditingModeAnimated:YES];
  [self.collectionView performBatchUpdates:^{
    NSInteger readSection = [self.collectionViewModel
        sectionForSectionIdentifier:SectionIdentifierRead];
    [self.collectionView
        deleteSections:[NSIndexSet indexSetWithIndex:readSection]];
    [self.collectionViewModel
        removeSectionWithIdentifier:SectionIdentifierRead];
  }
      completion:^(BOOL) {
        // Reload data to take into account possible sync events.
        [self applyPendingUpdates];
      }];
  // As we modified the section in the batch update block, remove the section in
  // another block.
  [self removeEmptySections];
}

- (void)deleteItemsAtIndexPaths:(NSArray*)indexPaths {
  [self updateIndexPaths:indexPaths
       usingEntryUpdater:^(CollectionViewItem* item) {
         [self.dataSource removeEntryFromItem:item];
       }];

  [self exitEditingModeAnimated:YES];

  [self.collectionView performBatchUpdates:^{
    [self collectionView:self.collectionView
        willDeleteItemsAtIndexPaths:indexPaths];

    [self.collectionView deleteItemsAtIndexPaths:indexPaths];
  }
      completion:^(BOOL) {
        // Reload data to take into account possible sync events.
        [self applyPendingUpdates];
      }];
  // As we modified the section in the batch update block, remove the section in
  // another block.
  [self removeEmptySections];
}

- (void)updateItemsInSectionIdentifier:(SectionIdentifier)identifier
                     usingEntryUpdater:(EntryUpdater)updater {
  [self.dataSource beginBatchUpdates];
  NSArray* readItems =
      [self.collectionViewModel itemsInSectionWithIdentifier:identifier];
  // Read the objects in reverse order to keep the order (last modified first).
  for (id item in [readItems reverseObjectEnumerator]) {
    if (updater)
      updater(item);
  }
  [self.dataSource endBatchUpdates];
}

- (void)updateIndexPaths:(NSArray<NSIndexPath*>*)indexPaths
       usingEntryUpdater:(EntryUpdater)updater {
  [self.dataSource beginBatchUpdates];
  // Read the objects in reverse order to keep the order (last modified first).
  for (NSIndexPath* index in [indexPaths reverseObjectEnumerator]) {
    CollectionViewItem* item = [self.collectionViewModel itemAtIndexPath:index];
    if (updater)
      updater(item);
  }
  [self.dataSource endBatchUpdates];
}

- (void)moveItemsFromSection:(SectionIdentifier)sourceSectionIdentifier
                   toSection:(SectionIdentifier)destinationSectionIdentifier {
  NSInteger sourceSection = [self.collectionViewModel
      sectionForSectionIdentifier:sourceSectionIdentifier];
  NSInteger numberOfSourceItems =
      [self.collectionViewModel numberOfItemsInSection:sourceSection];

  NSMutableArray* sortedIndexPaths = [NSMutableArray array];

  for (int index = 0; index < numberOfSourceItems; index++) {
    NSIndexPath* itemIndex =
        [NSIndexPath indexPathForItem:index inSection:sourceSection];
    [sortedIndexPaths addObject:itemIndex];
  }

  [self moveSelectedItems:sortedIndexPaths
                toSection:destinationSectionIdentifier];
}

- (void)moveSelectedItems:(NSArray*)sortedIndexPaths
                toSection:(SectionIdentifier)sectionIdentifier {
  // Reconfigure cells, allowing the custom actions to be updated.
  [self reconfigureCellsAtIndexPaths:sortedIndexPaths];

  NSInteger sectionCreatedIndex = [self initializeSection:sectionIdentifier];

  [self.collectionView performBatchUpdates:^{
    NSInteger section = [self.collectionViewModel
        sectionForSectionIdentifier:sectionIdentifier];

    NSInteger newItemIndex = 0;
    for (NSIndexPath* index in sortedIndexPaths) {
      // The |sortedIndexPaths| is a copy of the index paths before the
      // destination section has been added if necessary. The section part of
      // the index potentially needs to be updated.
      NSInteger updatedSection = index.section;
      if (updatedSection >= sectionCreatedIndex)
        updatedSection++;
      if (updatedSection == section) {
        // The item is already in the targeted section, there is no need to move
        // it.
        continue;
      }

      NSIndexPath* updatedIndex =
          [NSIndexPath indexPathForItem:index.item inSection:updatedSection];
      NSIndexPath* indexForModel =
          [NSIndexPath indexPathForItem:index.item - newItemIndex
                              inSection:updatedSection];

      // Index of the item in the new section. The newItemIndex is the index of
      // this item in the targeted section.
      NSIndexPath* newIndexPath =
          [NSIndexPath indexPathForItem:newItemIndex++ inSection:section];

      [self collectionView:self.collectionView
          willMoveItemAtIndexPath:indexForModel
                      toIndexPath:newIndexPath];
      [self.collectionView moveItemAtIndexPath:updatedIndex
                                   toIndexPath:newIndexPath];
    }
  }
      completion:^(BOOL) {
        // Reload data to take into account possible sync events.
        [self applyPendingUpdates];
      }];
  // As we modified the section in the batch update block, remove the section in
  // another block.
  [self removeEmptySections];
}

- (NSInteger)initializeSection:(SectionIdentifier)sectionIdentifier {
  if (![self.collectionViewModel
          hasSectionForSectionIdentifier:sectionIdentifier]) {
    // The new section IndexPath will be 1 if it is the read section with
    // items in the unread section, 0 otherwise.
    BOOL hasNonEmptySectionAbove =
        sectionIdentifier == SectionIdentifierRead &&
        [self hasItemInSection:SectionIdentifierUnread];
    NSInteger sectionIndex = hasNonEmptySectionAbove ? 1 : 0;

    [self.collectionView performBatchUpdates:^{
      [self.collectionViewModel insertSectionWithIdentifier:sectionIdentifier
                                                    atIndex:sectionIndex];

      [self.collectionViewModel
                         setHeader:[self headerForSection:sectionIdentifier]
          forSectionWithIdentifier:sectionIdentifier];

      [self.collectionView
          insertSections:[NSIndexSet indexSetWithIndex:sectionIndex]];
    }
                                  completion:nil];

    return sectionIndex;
  }
  return NSIntegerMax;
}

- (CollectionViewTextItem*)headerForSection:
    (SectionIdentifier)sectionIdentifier {
  CollectionViewTextItem* header =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeHeader];

  switch (sectionIdentifier) {
    case SectionIdentifierRead:
      header.text = l10n_util::GetNSString(IDS_IOS_READING_LIST_READ_HEADER);
      break;
    case SectionIdentifierUnread:
      header.text = l10n_util::GetNSString(IDS_IOS_READING_LIST_UNREAD_HEADER);
      break;
  }
  header.textColor = [[MDCPalette greyPalette] tint500];
  return header;
}

- (void)removeEmptySections {
  [self.collectionView performBatchUpdates:^{

    SectionIdentifier a[] = {SectionIdentifierRead, SectionIdentifierUnread};
    for (size_t i = 0; i < arraysize(a); i++) {
      SectionIdentifier sectionIdentifier = a[i];

      if ([self.collectionViewModel
              hasSectionForSectionIdentifier:sectionIdentifier] &&
          ![self hasItemInSection:sectionIdentifier]) {
        NSInteger section = [self.collectionViewModel
            sectionForSectionIdentifier:sectionIdentifier];

        [self.collectionView
            deleteSections:[NSIndexSet indexSetWithIndex:section]];
        [self.collectionViewModel
            removeSectionWithIdentifier:sectionIdentifier];
      }
    }
  }
                                completion:nil];
  if (!self.dataSource.hasElements) {
    [self collectionIsEmpty];
  } else {
    [_toolbar setHasReadItem:self.dataSource.hasRead];
  }
}

- (void)exitEditingModeAnimated:(BOOL)animated {
  [_actionSheet stop];
  [self.editor setEditing:NO animated:animated];
  [_toolbar setEditing:NO];
}

@end

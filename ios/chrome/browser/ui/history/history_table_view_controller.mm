// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/history/history_table_view_controller.h"

#include "base/i18n/time_formatting.h"
#include "base/mac/foundation_util.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "components/url_formatter/url_formatter.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/metrics/new_tab_page_uma.h"
#include "ios/chrome/browser/sync/sync_setup_service.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#import "ios/chrome/browser/ui/context_menu/context_menu_coordinator.h"
#include "ios/chrome/browser/ui/history/history_entries_status_item.h"
#import "ios/chrome/browser/ui/history/history_entries_status_item_delegate.h"
#include "ios/chrome/browser/ui/history/history_entry_inserter.h"
#import "ios/chrome/browser/ui/history/history_entry_item.h"
#include "ios/chrome/browser/ui/history/history_local_commands.h"
#import "ios/chrome/browser/ui/history/history_ui_constants.h"
#include "ios/chrome/browser/ui/history/history_util.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_cells_constants.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_text_item.h"
#import "ios/chrome/browser/ui/table_view/table_view_navigation_controller_constants.h"
#import "ios/chrome/browser/ui/url_loader.h"
#import "ios/chrome/browser/ui/util/pasteboard_util.h"
#import "ios/chrome/browser/ui/util/top_view_controller.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/referrer.h"
#import "ios/web/public/web_state/context_menu_params.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using history::BrowsingHistoryService;

namespace {
typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeHistoryEntry = kItemTypeEnumZero,
  ItemTypeEntriesStatus,
  ItemTypeActivityIndicator,
};
// Section identifier for the header (sync information) section.
const NSInteger kEntriesStatusSectionIdentifier = kSectionIdentifierEnumZero;
// Maximum number of entries to retrieve in a single query to history service.
const int kMaxFetchCount = 100;
}

@interface HistoryTableViewController ()<HistoryEntriesStatusItemDelegate,
                                         HistoryEntryInserterDelegate,
                                         UISearchResultsUpdating,
                                         UISearchBarDelegate> {
  // Closure to request next page of history.
  base::OnceClosure _query_history_continuation;
}

// Object to manage insertion of history entries into the table view model.
@property(nonatomic, strong) HistoryEntryInserter* entryInserter;
// Coordinator for displaying context menus for history entries.
@property(nonatomic, strong) ContextMenuCoordinator* contextMenuCoordinator;
// The current query for visible history entries.
@property(nonatomic, copy) NSString* currentQuery;
// YES if there are no results to show.
@property(nonatomic, assign) BOOL empty;
// YES if the history panel should show a notice about additional forms of
// browsing history.
@property(nonatomic, assign)
    BOOL shouldShowNoticeAboutOtherFormsOfBrowsingHistory;
// YES if there is an outstanding history query.
@property(nonatomic, assign, getter=isLoading) BOOL loading;
// YES if there are no more history entries to load.
@property(nonatomic, assign, getter=hasFinishedLoading) BOOL finishedLoading;
// YES if the table should be filtered by the next received query result.
@property(nonatomic, assign) BOOL filterQueryResult;
// This ViewController's searchController;
@property(nonatomic, strong) UISearchController* searchController;
// NavigationController UIToolbar Buttons.
@property(nonatomic, strong) UIBarButtonItem* cancelButton;
@property(nonatomic, strong) UIBarButtonItem* clearBrowsingDataButton;
@property(nonatomic, strong) UIBarButtonItem* deleteButton;
@property(nonatomic, strong) UIBarButtonItem* editButton;
@end

@implementation HistoryTableViewController
@synthesize browserState = _browserState;
@synthesize cancelButton = _cancelButton;
@synthesize clearBrowsingDataButton = _clearBrowsingDataButton;
@synthesize contextMenuCoordinator = _contextMenuCoordinator;
@synthesize currentQuery = _currentQuery;
@synthesize deleteButton = _deleteButton;
@synthesize editButton = _editButton;
@synthesize empty = _empty;
@synthesize entryInserter = _entryInserter;
@synthesize filterQueryResult = _filterQueryResult;
@synthesize finishedLoading = _finishedLoading;
@synthesize historyService = _historyService;
@synthesize loader = _loader;
@synthesize loading = _loading;
@synthesize localDispatcher = _localDispatcher;
@synthesize searchController = _searchController;
@synthesize shouldShowNoticeAboutOtherFormsOfBrowsingHistory =
    _shouldShowNoticeAboutOtherFormsOfBrowsingHistory;

#pragma mark - ViewController Lifecycle.

- (instancetype)init {
  return [super initWithTableViewStyle:UITableViewStylePlain
                           appBarStyle:ChromeTableViewControllerStyleNoAppBar];
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [self loadModel];

  // TableView configuration
  self.tableView.estimatedRowHeight = 56;
  self.tableView.rowHeight = UITableViewAutomaticDimension;
  self.tableView.estimatedSectionHeaderHeight = 56;
  self.tableView.sectionFooterHeight = 0.0;
  self.tableView.keyboardDismissMode = UIScrollViewKeyboardDismissModeOnDrag;
  self.tableView.allowsMultipleSelectionDuringEditing = YES;
  self.clearsSelectionOnViewWillAppear = NO;
  self.tableView.allowsMultipleSelection = YES;

  // ContextMenu gesture recognizer.
  UILongPressGestureRecognizer* longPressRecognizer = [
      [UILongPressGestureRecognizer alloc]
      initWithTarget:self
              action:@selector(displayContextMenuInvokedByGestureRecognizer:)];
  [self.tableView addGestureRecognizer:longPressRecognizer];

  // If the NavigationBar is not translucent, set
  // |self.extendedLayoutIncludesOpaqueBars| to YES in order to avoid a top
  // margin inset on the |_tableViewController| subview.
  self.extendedLayoutIncludesOpaqueBars = YES;

  // NavigationController configuration.
  self.title = l10n_util::GetNSString(IDS_HISTORY_TITLE);
  // Configures NavigationController Toolbar buttons.
  [self configureViewsForNonEditModeWithAnimation:NO];
  // Adds the "Done" button and hooks it up to |dismissHistory|.
  UIBarButtonItem* dismissButton = [[UIBarButtonItem alloc]
      initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                           target:self
                           action:@selector(dismissHistory)];
  [dismissButton setAccessibilityIdentifier:
                     kHistoryNavigationControllerDoneButtonIdentifier];
  self.navigationItem.rightBarButtonItem = dismissButton;

  // SearchController Configuration.
  // Init the searchController with nil so the results are displayed on the same
  // TableView.
  self.searchController =
      [[UISearchController alloc] initWithSearchResultsController:nil];
  self.searchController.dimsBackgroundDuringPresentation = NO;
  self.searchController.searchBar.delegate = self;
  self.searchController.searchResultsUpdater = self;
  self.searchController.searchBar.backgroundColor = [UIColor whiteColor];
  self.searchController.searchBar.accessibilityIdentifier =
      l10n_util::GetNSStringWithFixup(IDS_IOS_ICON_SEARCH);
  // UIKit needs to know which controller will be presenting the
  // searchController. If we don't add this trying to dismiss while
  // SearchController is active will fail.
  self.definesPresentationContext = YES;

  // For iOS 11 and later, place the search bar in the navigation bar. Otherwise
  // place the search bar in the table view's header.
  if (@available(iOS 11, *)) {
    self.navigationItem.searchController = self.searchController;
    self.navigationItem.hidesSearchBarWhenScrolling = NO;
  } else {
    self.tableView.tableHeaderView = self.searchController.searchBar;
  }
}

#pragma mark - TableViewModel

- (void)loadModel {
  [super loadModel];
  // Add initial info section as header.
  [self.tableViewModel
      addSectionWithIdentifier:kEntriesStatusSectionIdentifier];
  // TODO(crbug.com/833623): Temporary loading indicator, will update once we
  // decide on a standard.
  TableViewTextItem* entriesStatusItem =
      [[TableViewTextItem alloc] initWithType:ItemTypeEntriesStatus];
  entriesStatusItem.text = @"Loading";
  entriesStatusItem.textColor = TextItemColorBlack;
  [self.tableViewModel addItem:entriesStatusItem
       toSectionWithIdentifier:kEntriesStatusSectionIdentifier];

  _entryInserter =
      [[HistoryEntryInserter alloc] initWithModel:self.tableViewModel];
  _entryInserter.delegate = self;
  _empty = YES;
  [self showHistoryMatchingQuery:nil];
}

#pragma mark - Protocols

#pragma mark HistoryConsumer

- (void)historyQueryWasCompletedWithResults:
            (const std::vector<BrowsingHistoryService::HistoryEntry>&)results
                           queryResultsInfo:
                               (const BrowsingHistoryService::QueryResultsInfo&)
                                   queryResultsInfo
                        continuationClosure:
                            (base::OnceClosure)continuationClosure {
  self.loading = NO;
  _query_history_continuation = std::move(continuationClosure);

  // If history sync is enabled and there hasn't been a response from synced
  // history, try fetching again.
  SyncSetupService* syncSetupService =
      SyncSetupServiceFactory::GetForBrowserState(_browserState);
  if (syncSetupService->IsSyncEnabled() &&
      syncSetupService->IsDataTypeActive(syncer::HISTORY_DELETE_DIRECTIVES) &&
      queryResultsInfo.sync_timed_out) {
    [self showHistoryMatchingQuery:_currentQuery];
    return;
  }

  // If there are no results and no URLs have been loaded, report that no
  // history entries were found.
  if (results.empty() && self.empty) {
    [self updateEntriesStatusMessage];
    [self updateToolbarButtons];
    return;
  }

  self.finishedLoading = queryResultsInfo.reached_beginning;
  self.empty = NO;

  // Header section should be updated outside of batch updates, otherwise
  // loading indicator removal will not be observed.
  [self updateEntriesStatusMessage];

  NSMutableArray* resultsItems = [NSMutableArray array];
  NSString* searchQuery =
      [base::SysUTF16ToNSString(queryResultsInfo.search_text) copy];

  void (^tableUpdates)(void) = ^{
    // There should always be at least a header section present.
    DCHECK([[self tableViewModel] numberOfSections]);
    for (const BrowsingHistoryService::HistoryEntry& entry : results) {
      HistoryEntryItem* item =
          [[HistoryEntryItem alloc] initWithType:ItemTypeHistoryEntry];
      item.text = [history::FormattedTitle(entry.title, entry.url) copy];
      item.detailText =
          [base::SysUTF8ToNSString(entry.url.GetOrigin().spec()) copy];
      item.timeText = [base::SysUTF16ToNSString(
          base::TimeFormatTimeOfDay(entry.time)) copy];
      item.URL = entry.url;
      item.timestamp = entry.time;
      [resultsItems addObject:item];
    }

    [self updateToolbarButtons];

    if ((self.searchController.isActive && [searchQuery length] > 0 &&
         [self.currentQuery isEqualToString:searchQuery]) ||
        self.filterQueryResult) {
      // If in search mode, filter out entries that are not part of the
      // search result.
      [self filterForHistoryEntries:resultsItems];
      NSArray* deletedIndexPaths = self.tableView.indexPathsForSelectedRows;
      [self deleteItemsFromTableViewModelWithIndex:deletedIndexPaths];
      self.filterQueryResult = NO;
    }
    // Wait to insert until after the deletions are done, this is needed
    // because performBatchUpdates processes deletion indexes first, and
    // then inserts.
    for (HistoryEntryItem* item in resultsItems) {
      [self.entryInserter insertHistoryEntryItem:item];
    }
  };

  // If iOS11+ use performBatchUpdates: instead of beginUpdates/endUpdates.
  if (@available(iOS 11, *)) {
    [self.tableView performBatchUpdates:tableUpdates
                             completion:^(BOOL) {
                               [self updateTableViewAfterDeletingEntries];
                             }];
  } else {
    [self.tableView beginUpdates];
    tableUpdates();
    [self updateTableViewAfterDeletingEntries];
    [self.tableView endUpdates];
  }
}

- (void)showNoticeAboutOtherFormsOfBrowsingHistory:(BOOL)shouldShowNotice {
  self.shouldShowNoticeAboutOtherFormsOfBrowsingHistory = shouldShowNotice;
  // Update the history entries status message if there is no query in progress.
  if (!self.isLoading) {
    [self updateEntriesStatusMessage];
  }
}

- (void)historyWasDeleted {
  // If history has been deleted, reload history filtering for the current
  // results. This only observes local changes to history, i.e. removing
  // history via the clear browsing data page.
  self.filterQueryResult = YES;
  [self showHistoryMatchingQuery:nil];
}

#pragma mark HistoryEntriesStatusItemDelegate

- (void)historyEntriesStatusItem:(HistoryEntriesStatusItem*)item
               didRequestOpenURL:(const GURL&)URL {
  // TODO(crbug.com/805190): Migrate. This will navigate to the status message
  // "Show Full History" URL.
}

#pragma mark HistoryEntryInserterDelegate

- (void)historyEntryInserter:(HistoryEntryInserter*)inserter
    didInsertItemAtIndexPath:(NSIndexPath*)indexPath {
  [self.tableView insertRowsAtIndexPaths:@[ indexPath ]
                        withRowAnimation:UITableViewRowAnimationNone];
}

- (void)historyEntryInserter:(HistoryEntryInserter*)inserter
     didInsertSectionAtIndex:(NSInteger)sectionIndex {
  [self.tableView insertSections:[NSIndexSet indexSetWithIndex:sectionIndex]
                withRowAnimation:UITableViewRowAnimationNone];
}

- (void)historyEntryInserter:(HistoryEntryInserter*)inserter
     didRemoveSectionAtIndex:(NSInteger)sectionIndex {
  [self.tableView deleteSections:[NSIndexSet indexSetWithIndex:sectionIndex]
                withRowAnimation:UITableViewRowAnimationNone];
}

#pragma mark HistoryEntryItemDelegate
// TODO(crbug.com/805190): Migrate once we decide how to handle favicons and the
// a11y callback on HistoryEntryItem.

#pragma mark UISearchResultsUpdating

- (void)updateSearchResultsForSearchController:
    (UISearchController*)searchController {
  DCHECK_EQ(self.searchController, searchController);
  [self showHistoryMatchingQuery:searchController.searchBar.text];
}

#pragma mark UISearchBarDelegate

- (void)searchBarTextDidBeginEditing:(UISearchBar*)searchBar {
  [self updateEntriesStatusMessage];
}

- (void)searchBarTextDidEndEditing:(UISearchBar*)searchBar {
  [self updateEntriesStatusMessage];
}

#pragma mark - History Data Updates

// Search history for text |query| and display the results. |query| may be nil.
// If query is empty, show all history items.
- (void)showHistoryMatchingQuery:(NSString*)query {
  self.finishedLoading = NO;
  self.currentQuery = query;
  [self fetchHistoryForQuery:query continuation:false];
}

// Deletes selected items from browser history and removes them from the
// tableView.
- (void)deleteSelectedItemsFromHistory {
  NSArray* toDeleteIndexPaths = self.tableView.indexPathsForSelectedRows;

  // Delete items from Browser History.
  std::vector<BrowsingHistoryService::HistoryEntry> entries;
  for (NSIndexPath* indexPath in toDeleteIndexPaths) {
    HistoryEntryItem* object = base::mac::ObjCCastStrict<HistoryEntryItem>(
        [self.tableViewModel itemAtIndexPath:indexPath]);
    BrowsingHistoryService::HistoryEntry entry;
    entry.url = object.URL;
    // TODO(crbug.com/634507) Remove base::TimeXXX::ToInternalValue().
    entry.all_timestamps.insert(object.timestamp.ToInternalValue());
    entries.push_back(entry);
  }
  self.historyService->RemoveVisits(entries);

  // Delete items from |self.tableView|.
  // If iOS11+ use performBatchUpdates: instead of beginUpdates/endUpdates.
  if (@available(iOS 11, *)) {
    [self.tableView performBatchUpdates:^{
      [self deleteItemsFromTableViewModelWithIndex:toDeleteIndexPaths];
    }
        completion:^(BOOL) {
          [self updateTableViewAfterDeletingEntries];
          [self configureViewsForNonEditModeWithAnimation:YES];
        }];
  } else {
    [self.tableView beginUpdates];
    [self deleteItemsFromTableViewModelWithIndex:toDeleteIndexPaths];
    [self updateTableViewAfterDeletingEntries];
    [self configureViewsForNonEditModeWithAnimation:YES];
    [self.tableView endUpdates];
  }
}

#pragma mark - UITableViewDelegate

- (CGFloat)tableView:(UITableView*)tableView
    heightForHeaderInSection:(NSInteger)section {
  if (section ==
      [self.tableViewModel
          sectionForSectionIdentifier:kEntriesStatusSectionIdentifier])
    return 0;
  return UITableViewAutomaticDimension;
}

- (void)tableView:(UITableView*)tableView
    didSelectRowAtIndexPath:(NSIndexPath*)indexPath {
  DCHECK_EQ(tableView, self.tableView);
  if (self.isEditing) {
    [self updateToolbarButtons];
  } else {
    TableViewItem* item = [self.tableViewModel itemAtIndexPath:indexPath];
    // Only navigate and record metrics if a ItemTypeHistoryEntry was selected.
    if (item.type == ItemTypeHistoryEntry) {
      if (self.searchController.isActive) {
        // Set the searchController active property to NO or the SearchBar will
        // cause the navigation controller to linger for a second  when
        // dismissing.
        self.searchController.active = NO;
        base::RecordAction(
            base::UserMetricsAction("HistoryPage_SearchResultClick"));
      } else {
        base::RecordAction(
            base::UserMetricsAction("HistoryPage_EntryLinkClick"));
      }
      HistoryEntryItem* historyItem =
          base::mac::ObjCCastStrict<HistoryEntryItem>(item);
      [self openURL:historyItem.URL];
    }
  }
}

- (void)tableView:(UITableView*)tableView
    didDeselectRowAtIndexPath:(NSIndexPath*)indexPath {
  DCHECK_EQ(tableView, self.tableView);
  if (self.editing)
    [self updateToolbarButtons];
}

- (UITableViewCell*)tableView:(UITableView*)tableView
        cellForRowAtIndexPath:(NSIndexPath*)indexPath {
  UITableViewCell* cellToReturn =
      [super tableView:tableView cellForRowAtIndexPath:indexPath];
  TableViewItem* item = [self.tableViewModel itemAtIndexPath:indexPath];
  if (item.type == ItemTypeEntriesStatus) {
    cellToReturn.userInteractionEnabled = NO;
  }
  return cellToReturn;
}

- (BOOL)tableView:(UITableView*)tableView
    canEditRowAtIndexPath:(NSIndexPath*)indexPath {
  TableViewItem* item = [self.tableViewModel itemAtIndexPath:indexPath];
  if (item.type == ItemTypeEntriesStatus) {
    return NO;
  } else {
    return YES;
  }
}

#pragma mark - UIScrollViewDelegate

- (void)scrollViewDidScroll:(UIScrollView*)scrollView {
  [super scrollViewDidScroll:scrollView];

  if (self.hasFinishedLoading)
    return;

  CGFloat insetHeight =
      scrollView.contentInset.top + scrollView.contentInset.bottom;
  CGFloat contentViewHeight = scrollView.bounds.size.height - insetHeight;
  CGFloat contentHeight = scrollView.contentSize.height;
  CGFloat contentOffset = scrollView.contentOffset.y;
  CGFloat buffer = contentViewHeight;
  // If the scroll view is approaching the end of loaded history, try to fetch
  // more history. Do so when the content offset is greater than the content
  // height minus the view height, minus a buffer to start the fetch early.
  if (contentOffset > (contentHeight - contentViewHeight) - buffer &&
      !self.isLoading) {
    // If at end, try to grab more history.
    NSInteger lastSection = [self.tableViewModel numberOfSections] - 1;
    NSInteger lastItemIndex =
        [self.tableViewModel numberOfItemsInSection:lastSection] - 1;
    if (lastSection == 0 || lastItemIndex < 0) {
      return;
    }

    [self fetchHistoryForQuery:_currentQuery continuation:true];
  }
}

#pragma mark - Private methods

// Fetches history for search text |query|. If |query| is nil or the empty
// string, all history is fetched. If continuation is false, then the most
// recent results are fetched, otherwise the results more recent than the
// previous query will be returned.
- (void)fetchHistoryForQuery:(NSString*)query continuation:(BOOL)continuation {
  self.loading = YES;
  // Add loading indicator if no items are shown.
  if (self.empty && !self.searchController.isActive) {
    [self addLoadingIndicator];
  }

  if (continuation) {
    DCHECK(_query_history_continuation);
    std::move(_query_history_continuation).Run();
  } else {
    _query_history_continuation.Reset();

    BOOL fetchAllHistory = !query || [query isEqualToString:@""];
    base::string16 queryString =
        fetchAllHistory ? base::string16() : base::SysNSStringToUTF16(query);
    history::QueryOptions options;
    options.duplicate_policy =
        fetchAllHistory ? history::QueryOptions::REMOVE_DUPLICATES_PER_DAY
                        : history::QueryOptions::REMOVE_ALL_DUPLICATES;
    options.max_count = kMaxFetchCount;
    options.matching_algorithm =
        query_parser::MatchingAlgorithm::ALWAYS_PREFIX_SEARCH;
    self.historyService->QueryHistory(queryString, options);
  }
}

// Updates various elements after history items have been deleted from the
// TableView.
- (void)updateTableViewAfterDeletingEntries {
  // If only the header section remains, there are no history entries.
  if ([self.tableViewModel numberOfSections] == 1) {
    self.empty = YES;
  }
  [self updateEntriesStatusMessage];
  [self updateToolbarButtons];
}

// Updates header section to provide relevant information about the currently
// displayed history entries. There should only ever be at most one item in this
// section.
- (void)updateEntriesStatusMessage {
  NSString* messageText = nil;
  TextItemColor messageColor;
  if (self.empty) {
    messageText = self.searchController.isActive
                      ? l10n_util::GetNSString(IDS_HISTORY_NO_SEARCH_RESULTS)
                      : l10n_util::GetNSString(IDS_HISTORY_NO_RESULTS);
    messageColor = TextItemColorBlack;
  } else if (self.shouldShowNoticeAboutOtherFormsOfBrowsingHistory &&
             !self.searchController.isActive) {
    messageText =
        l10n_util::GetNSString(IDS_IOS_HISTORY_OTHER_FORMS_OF_HISTORY);
    messageColor = TextItemColorLightGrey;
  }

  // Get the number of items currently at the StatusMessageSection.
  NSArray* items = [self.tableViewModel
      itemsInSectionWithIdentifier:kEntriesStatusSectionIdentifier];
  DCHECK([items count] <= 1);

  // If no message remove message cell/item if exists, then return.
  if (messageText == nil) {
    if ([items count]) {
      NSIndexPath* statusMessageIndexPath = [self.tableViewModel
          indexPathForItemType:ItemTypeEntriesStatus
             sectionIdentifier:kEntriesStatusSectionIdentifier];
      [self.tableViewModel removeItemWithType:ItemTypeEntriesStatus
                    fromSectionWithIdentifier:kEntriesStatusSectionIdentifier];
      [self.tableView deleteRowsAtIndexPaths:@[ statusMessageIndexPath ]
                            withRowAnimation:UITableViewRowAnimationNone];
    }
    return;
  }

  if ([items count]) {
    // If a previous item exists, update its message.
    TableViewItem* oldEntriesStatusItem = items[0];
    TableViewTextItem* oldEntriesStatusTextItem =
        base::mac::ObjCCastStrict<TableViewTextItem>(oldEntriesStatusItem);
    // If its the same message there's no need to update the item or reload the
    // table.
    if ([messageText isEqualToString:oldEntriesStatusTextItem.text])
      return;
    oldEntriesStatusTextItem.text = messageText;
    oldEntriesStatusTextItem.textColor = messageColor;
    NSIndexPath* statusMessageIndexPath = [self.tableViewModel
        indexPathForItemType:ItemTypeEntriesStatus
           sectionIdentifier:kEntriesStatusSectionIdentifier];
    [self.tableView reloadRowsAtIndexPaths:@[ statusMessageIndexPath ]
                          withRowAnimation:UITableViewRowAnimationNone];
  } else {
    // If a previous item doesn't exist it create a new one and insert it.
    TableViewTextItem* entriesStatusItem =
        [[TableViewTextItem alloc] initWithType:ItemTypeEntriesStatus];
    entriesStatusItem.text = messageText;
    entriesStatusItem.textColor = messageColor;
    [self.tableViewModel addItem:entriesStatusItem
         toSectionWithIdentifier:kEntriesStatusSectionIdentifier];
    NSIndexPath* statusMessageIndexPath =
        [self.tableViewModel indexPathForItem:entriesStatusItem];
    [self.tableView insertRowsAtIndexPaths:@[ statusMessageIndexPath ]
                          withRowAnimation:UITableViewRowAnimationNone];
  }
}

// Deletes all items in the tableView which indexes are included in indexArray,
// needs to be run inside a performBatchUpdates block.
- (void)deleteItemsFromTableViewModelWithIndex:(NSArray*)indexArray {
  NSArray* sortedIndexPaths =
      [indexArray sortedArrayUsingSelector:@selector(compare:)];
  for (NSIndexPath* indexPath in [sortedIndexPaths reverseObjectEnumerator]) {
    NSInteger sectionIdentifier =
        [self.tableViewModel sectionIdentifierForSection:indexPath.section];
    NSInteger itemType = [self.tableViewModel itemTypeForIndexPath:indexPath];
    NSUInteger index =
        [self.tableViewModel indexInItemTypeForIndexPath:indexPath];
    [self.tableViewModel removeItemWithType:itemType
                  fromSectionWithIdentifier:sectionIdentifier
                                    atIndex:index];
  }
  [self.tableView deleteRowsAtIndexPaths:indexArray
                        withRowAnimation:UITableViewRowAnimationNone];

  // Remove any empty sections, except the header section.
  for (int section = self.tableView.numberOfSections - 1; section > 0;
       --section) {
    if (![self.tableViewModel numberOfItemsInSection:section]) {
      [self.entryInserter removeSection:section];
    }
  }
}

// Selects all items in the tableView that are not included in entries.
- (void)filterForHistoryEntries:(NSArray*)entries {
  for (int section = 1; section < [self.tableViewModel numberOfSections];
       ++section) {
    NSInteger sectionIdentifier =
        [self.tableViewModel sectionIdentifierForSection:section];
    if ([self.tableViewModel
            hasSectionForSectionIdentifier:sectionIdentifier]) {
      NSArray* items =
          [self.tableViewModel itemsInSectionWithIdentifier:sectionIdentifier];
      for (id item in items) {
        HistoryEntryItem* historyItem =
            base::mac::ObjCCastStrict<HistoryEntryItem>(item);
        if (![entries containsObject:historyItem]) {
          NSIndexPath* indexPath =
              [self.tableViewModel indexPathForItem:historyItem];
          [self.tableView selectRowAtIndexPath:indexPath
                                      animated:NO
                                scrollPosition:UITableViewScrollPositionNone];
        }
      }
    }
  }
}

// Adds loading indicator to the top of the history tableView, if one is not
// already present.
- (void)addLoadingIndicator {
  // TODO(crbug.com/805190): Migrate.
}

#pragma mark Navigation Toolbar Configuration

// Animates the view configuration after flipping the current status of |[self
// setEditing]|.
- (void)animateViewsConfigurationForEditingChange {
  if (self.isEditing) {
    [self configureViewsForNonEditModeWithAnimation:YES];
  } else {
    [self configureViewsForEditModeWithAnimation:YES];
  }
}

// Default TableView and NavigationBar UIToolbar configuration.
- (void)configureViewsForNonEditModeWithAnimation:(BOOL)animated {
  [self setEditing:NO animated:animated];
  UIBarButtonItem* spaceButton = [[UIBarButtonItem alloc]
      initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace
                           target:nil
                           action:nil];
  [self setToolbarItems:@[
    self.clearBrowsingDataButton, spaceButton, self.editButton
  ]
               animated:animated];
  [self updateToolbarButtons];
}

// Configures the TableView and NavigationBar UIToolbar for edit mode.
- (void)configureViewsForEditModeWithAnimation:(BOOL)animated {
  [self setEditing:YES animated:animated];
  UIBarButtonItem* spaceButton = [[UIBarButtonItem alloc]
      initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace
                           target:nil
                           action:nil];
  [self setToolbarItems:@[ self.deleteButton, spaceButton, self.cancelButton ]
               animated:animated];
  [self updateToolbarButtons];
}

// Updates the NavigationBar UIToolbar buttons.
- (void)updateToolbarButtons {
  self.deleteButton.enabled =
      [[self.tableView indexPathsForSelectedRows] count];
  self.editButton.enabled = !self.empty;
}

#pragma mark Context Menu

// Displays context menu on cell pressed with gestureRecognizer.
- (void)displayContextMenuInvokedByGestureRecognizer:
    (UILongPressGestureRecognizer*)gestureRecognizer {
  if (gestureRecognizer.numberOfTouches != 1 || self.editing ||
      gestureRecognizer.state != UIGestureRecognizerStateBegan) {
    return;
  }

  CGPoint touchLocation =
      [gestureRecognizer locationOfTouch:0 inView:self.tableView];
  NSIndexPath* touchedItemIndexPath =
      [self.tableView indexPathForRowAtPoint:touchLocation];
  // If there's no index path, or the index path is for the header item, do not
  // display a contextual menu.
  if (!touchedItemIndexPath ||
      [touchedItemIndexPath
          isEqual:[NSIndexPath indexPathForItem:0 inSection:0]])
    return;

  HistoryEntryItem* entry = base::mac::ObjCCastStrict<HistoryEntryItem>(
      [self.tableViewModel itemAtIndexPath:touchedItemIndexPath]);

  __weak HistoryTableViewController* weakSelf = self;
  web::ContextMenuParams params;
  params.location = touchLocation;
  params.view = self.tableView;
  NSString* menuTitle =
      base::SysUTF16ToNSString(url_formatter::FormatUrl(entry.URL));
  params.menu_title = [menuTitle copy];

  // Present sheet/popover using controller that is added to view hierarchy.
  // TODO(crbug.com/754642): Remove TopPresentedViewController().
  UIViewController* topController =
      top_view_controller::TopPresentedViewController();

  self.contextMenuCoordinator =
      [[ContextMenuCoordinator alloc] initWithBaseViewController:topController
                                                          params:params];

  // TODO(crbug.com/606503): Refactor context menu creation code to be shared
  // with BrowserViewController.
  // Add "Open in New Tab" option.
  NSString* openInNewTabTitle =
      l10n_util::GetNSStringWithFixup(IDS_IOS_CONTENT_CONTEXT_OPENLINKNEWTAB);
  ProceduralBlock openInNewTabAction = ^{
    [weakSelf openURLInNewTab:entry.URL];
  };
  [self.contextMenuCoordinator addItemWithTitle:openInNewTabTitle
                                         action:openInNewTabAction];

  // Add "Open in New Incognito Tab" option.
  NSString* openInNewIncognitoTabTitle = l10n_util::GetNSStringWithFixup(
      IDS_IOS_CONTENT_CONTEXT_OPENLINKNEWINCOGNITOTAB);
  ProceduralBlock openInNewIncognitoTabAction = ^{
    [weakSelf openURLInNewIncognitoTab:entry.URL];
  };
  [self.contextMenuCoordinator addItemWithTitle:openInNewIncognitoTabTitle
                                         action:openInNewIncognitoTabAction];

  // Add "Copy URL" option.
  NSString* copyURLTitle =
      l10n_util::GetNSStringWithFixup(IDS_IOS_CONTENT_CONTEXT_COPY);
  ProceduralBlock copyURLAction = ^{
    StoreURLInPasteboard(entry.URL);
  };
  [self.contextMenuCoordinator addItemWithTitle:copyURLTitle
                                         action:copyURLAction];
  [self.contextMenuCoordinator start];
}

// Opens URL in a new non-incognito tab and dismisses the history view.
- (void)openURLInNewTab:(const GURL&)URL {
  GURL copiedURL(URL);
  [self.localDispatcher dismissHistoryWithCompletion:^{
    [self.loader webPageOrderedOpen:copiedURL
                           referrer:web::Referrer()
                        inIncognito:NO
                       inBackground:NO
                           appendTo:kLastTab];
  }];
}

// Opens URL in a new incognito tab and dismisses the history view.
- (void)openURLInNewIncognitoTab:(const GURL&)URL {
  GURL copiedURL(URL);
  [self.localDispatcher dismissHistoryWithCompletion:^{
    [self.loader webPageOrderedOpen:copiedURL
                           referrer:web::Referrer()
                        inIncognito:YES
                       inBackground:NO
                           appendTo:kLastTab];
  }];
}

#pragma mark Helper Methods

// Opens URL in the current tab and dismisses the history view.
- (void)openURL:(const GURL&)URL {
  new_tab_page_uma::RecordAction(_browserState,
                                 new_tab_page_uma::ACTION_OPENED_HISTORY_ENTRY);
  web::NavigationManager::WebLoadParams params(URL);
  params.transition_type = ui::PAGE_TRANSITION_AUTO_BOOKMARK;
  [self.localDispatcher dismissHistoryWithCompletion:^{
    [self.loader loadURLWithParams:params];
  }];
}

// Dismisses this ViewController.
- (void)dismissHistory {
  [self.localDispatcher dismissHistoryWithCompletion:nil];
}

- (void)openPrivacySettings {
  // Ignore the button tap if |self| is presenting another ViewController.
  if ([self presentedViewController]) {
    return;
  }
  base::RecordAction(
      base::UserMetricsAction("HistoryPage_InitClearBrowsingData"));
  [self.localDispatcher displayPrivacySettings];
}

#pragma mark Setter & Getters

- (UIBarButtonItem*)cancelButton {
  if (!_cancelButton) {
    NSString* titleString =
        l10n_util::GetNSString(IDS_HISTORY_CANCEL_EDITING_BUTTON);
    _cancelButton = [[UIBarButtonItem alloc]
        initWithTitle:titleString
                style:UIBarButtonItemStylePlain
               target:self
               action:@selector(animateViewsConfigurationForEditingChange)];
    _cancelButton.accessibilityIdentifier =
        kHistoryToolbarCancelButtonIdentifier;
  }
  return _cancelButton;
}

// TODO(crbug.com/831865): Find a way to disable the button when a VC is
// presented.
- (UIBarButtonItem*)clearBrowsingDataButton {
  if (!_clearBrowsingDataButton) {
    NSString* titleString = l10n_util::GetNSStringWithFixup(
        IDS_HISTORY_OPEN_CLEAR_BROWSING_DATA_DIALOG);
    _clearBrowsingDataButton =
        [[UIBarButtonItem alloc] initWithTitle:titleString
                                         style:UIBarButtonItemStylePlain
                                        target:self
                                        action:@selector(openPrivacySettings)];
    _clearBrowsingDataButton.accessibilityIdentifier =
        kHistoryToolbarClearBrowsingButtonIdentifier;
    _clearBrowsingDataButton.tintColor = [UIColor redColor];
  }
  return _clearBrowsingDataButton;
}

- (UIBarButtonItem*)deleteButton {
  if (!_deleteButton) {
    NSString* titleString =
        l10n_util::GetNSString(IDS_HISTORY_DELETE_SELECTED_ENTRIES_BUTTON);
    _deleteButton = [[UIBarButtonItem alloc]
        initWithTitle:titleString
                style:UIBarButtonItemStylePlain
               target:self
               action:@selector(deleteSelectedItemsFromHistory)];
    _deleteButton.accessibilityIdentifier =
        kHistoryToolbarDeleteButtonIdentifier;
    _deleteButton.tintColor = [UIColor redColor];
  }
  return _deleteButton;
}

- (UIBarButtonItem*)editButton {
  if (!_editButton) {
    NSString* titleString =
        l10n_util::GetNSString(IDS_HISTORY_START_EDITING_BUTTON);
    _editButton = [[UIBarButtonItem alloc]
        initWithTitle:titleString
                style:UIBarButtonItemStylePlain
               target:self
               action:@selector(animateViewsConfigurationForEditingChange)];
    _editButton.accessibilityIdentifier = kHistoryToolbarEditButtonIdentifier;
  }
  return _editButton;
}

@end

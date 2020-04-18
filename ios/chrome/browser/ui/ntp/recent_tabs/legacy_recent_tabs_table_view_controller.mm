// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/recent_tabs/legacy_recent_tabs_table_view_controller.h"

#include <memory>

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/experimental_flags.h"
#import "ios/chrome/browser/metrics/new_tab_page_uma.h"
#include "ios/chrome/browser/sessions/tab_restore_service_delegate_impl_ios.h"
#include "ios/chrome/browser/sessions/tab_restore_service_delegate_impl_ios_factory.h"
#include "ios/chrome/browser/sync/ios_chrome_profile_sync_service_factory.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_configurator.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_consumer.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_mediator.h"
#include "ios/chrome/browser/ui/commands/application_commands.h"
#import "ios/chrome/browser/ui/commands/show_signin_command.h"
#import "ios/chrome/browser/ui/context_menu/context_menu_coordinator.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/legacy_recent_tabs_table_view_controller_delegate.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/recent_tabs_constants.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/recent_tabs_handset_view_controller.h"
#include "ios/chrome/browser/ui/ntp/recent_tabs/synced_sessions.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/generic_section_header_view.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/header_of_collapsable_section_protocol.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/session_section_header_view.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/session_tab_data_view.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/show_full_history_view.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/signed_in_sync_in_progress_view.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/signed_in_sync_off_view.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/signed_in_sync_on_no_sessions_view.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/spacers_view.h"
#import "ios/chrome/browser/ui/settings/sync_utils/sync_presenter.h"
#import "ios/chrome/browser/ui/signin_interaction/public/signin_presenter.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/url_loader.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/chrome/browser/ui/util/top_view_controller.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/referrer.h"
#import "ios/web/public/web_state/context_menu_params.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Key for saving whether the Other Device section is collapsed.
NSString* const kOtherDeviceCollapsedKey = @"OtherDevicesCollapsed";

// Key for saving whether the Recently Closed section is collapsed.
NSString* const kRecentlyClosedCollapsedKey = @"RecentlyClosedCollapsed";

// Tag to extract the section headers from the cells.
enum { kSectionHeader = 1 };

// Margin at the top of the sigin-in promo view.
const CGFloat kSigninPromoViewTopMargin = 24;

// Types of sections.
enum SectionType {
  SEPARATOR_SECTION,
  CLOSED_TAB_SECTION,
  OTHER_DEVICES_SECTION,
  SESSION_SECTION,
};

// Types of cells.
enum CellType {
  CELL_CLOSED_TAB_SECTION_HEADER,
  CELL_CLOSED_TAB_DATA,
  CELL_SHOW_FULL_HISTORY,
  CELL_SEPARATOR,
  CELL_OTHER_DEVICES_SECTION_HEADER,
  CELL_OTHER_DEVICES_SIGNED_IN_SYNC_OFF,
  CELL_OTHER_DEVICES_SIGNED_IN_SYNC_ON_NO_SESSIONS,
  CELL_OTHER_DEVICES_SIGNIN_PROMO,
  CELL_OTHER_DEVICES_SYNC_IN_PROGRESS,
  CELL_SESSION_SECTION_HEADER,
  CELL_SESSION_TAB_DATA,
};

}  // namespace

@interface LegacyRecentTabsTableViewController ()<SigninPromoViewConsumer,
                                                  SigninPresenter,
                                                  SyncPresenter> {
  ios::ChromeBrowserState* _browserState;  // weak
  // The service that manages the recently closed tabs.
  sessions::TabRestoreService* _tabRestoreService;  // weak
  // Loader used to open new tabs.
  __weak id<UrlLoader> _loader;
  // The sync state.
  SessionsSyncUserState _sessionState;
  // The synced sessions.
  std::unique_ptr<synced_sessions::SyncedSessions> _syncedSessions;
  // Handles displaying the context menu for all form factors.
  ContextMenuCoordinator* _contextMenuCoordinator;
  SigninPromoViewMediator* _signinPromoViewMediator;
}
// Returns the type of the section at index |section|.
- (SectionType)sectionType:(NSInteger)section;
// Returns the type of the cell at the path |indexPath|.
- (CellType)cellType:(NSIndexPath*)indexPath;
// Returns the index of a section based on |sectionType|.
- (NSInteger)sectionIndexForSectionType:(SectionType)sectionType;
// Returns the number of sections before the other devices or session sections.
- (NSInteger)numberOfSectionsBeforeSessionOrOtherDevicesSections;
// Dismisses the modal containing the Recent Tabs panel (iPhone only).
- (void)dismissRecentTabsModal;
// Opens a new tab with the content of |distantTab|.
- (void)openTabWithContentOfDistantTab:
    (synced_sessions::DistantTab const*)distantTab;
// Opens a new tab with |url|.
- (void)openTabWithURL:(const GURL&)url;
// Shows the user's full history.
- (void)showFullHistory;
// Deletes/inserts cells for section at index |sectionIndex|.
- (void)toggleExpansionOfSection:(NSInteger)sectionIndex;
// Returns the key used to map |distantSession| to a collapsed status.
- (NSString*)keyForDistantSession:
    (synced_sessions::DistantSession const*)distantSession;
// Sets whether the session addressed with |sectionKey| is collapsed.
- (void)setSection:(NSString*)sectionKey collapsed:(BOOL)collapsed;
// Returns whether the section addressed with |sectionKey| is collapsed.
- (BOOL)sectionIsCollapsed:(NSString*)sectionKey;
// Returns the number of session sections. Requires |_sessionState| to be
// USER_SIGNED_IN_SYNC_ON_WITH_SESSIONS.
- (NSInteger)numberOfSessionSections;
// Returns the section indexes of the Session section or the Other Devices
// section.
- (NSIndexSet*)sessionOrOtherDevicesSectionsIndexes;
// Returns the index of the session located at |indexPath|.
- (size_t)indexOfSessionAtIndexPath:(NSIndexPath*)indexPath;
// Returns the session at |indexPath|.
- (synced_sessions::DistantSession const*)sessionAtIndexPath:
    (NSIndexPath*)indexPath;
// Returns the session tab at the index |indexPath|.
- (synced_sessions::DistantTab const*)distantTabAtIndex:(NSIndexPath*)indexPath;
// Opens in new tabs all the tabs of the distant session at index |indexPath|.
- (void)openTabsFromSessionAtIndexPath:(NSIndexPath*)indexPath;
// Removes all the cells of the session section at index |indexPath|.
- (void)removeSessionAtIndexPath:(NSIndexPath*)indexPath;
// Handles long presses on the UITableView, possibly opening context menus.
- (void)handleLongPress:(UILongPressGestureRecognizer*)longPressGesture;

// The dispatcher used by this ViewController.
@property(nonatomic, readonly, weak) id<ApplicationCommands> dispatcher;

@end

@implementation LegacyRecentTabsTableViewController

@synthesize delegate = delegate_;
@synthesize dispatcher = _dispatcher;
@synthesize handsetCommandHandler = _handsetCommandHandler;

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                              loader:(id<UrlLoader>)loader
                          dispatcher:(id<ApplicationCommands>)dispatcher {
  self = [super initWithStyle:UITableViewStylePlain];
  if (self) {
    DCHECK(browserState);
    DCHECK(loader);
    _browserState = browserState;
    _loader = loader;
    _sessionState = SessionsSyncUserState::USER_SIGNED_OUT;
    _syncedSessions.reset(new synced_sessions::SyncedSessions());
    _dispatcher = dispatcher;
  }
  return self;
}

- (void)dealloc {
  [_signinPromoViewMediator signinPromoViewRemoved];
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.accessibilityIdentifier =
      kRecentTabsTableViewControllerAccessibilityIdentifier;
  self.tableView.rowHeight = UITableViewAutomaticDimension;
  self.tableView.estimatedRowHeight =
      [SessionTabDataView desiredHeightInUITableViewCell];
  [self.tableView setSeparatorColor:[UIColor clearColor]];
  [self.tableView setDataSource:self];
  [self.tableView setDelegate:self];
  UILongPressGestureRecognizer* longPress =
      [[UILongPressGestureRecognizer alloc]
          initWithTarget:self
                  action:@selector(handleLongPress:)];
  longPress.delegate = self;
  [self.tableView addGestureRecognizer:longPress];
}

- (SectionType)sectionType:(NSInteger)section {
  if (section == 0) {
    return CLOSED_TAB_SECTION;
  }
  if (section == 1) {
    return SEPARATOR_SECTION;
  }
  if (section < [self numberOfSectionsBeforeSessionOrOtherDevicesSections]) {
    return CLOSED_TAB_SECTION;
  }
  if (_sessionState ==
      SessionsSyncUserState::USER_SIGNED_IN_SYNC_ON_WITH_SESSIONS) {
    return SESSION_SECTION;
  }
  // Other cases of recent_tabs::USER_SIGNED_IN_SYNC_OFF,
  // recent_tabs::USER_SIGNED_IN_SYNC_ON_NO_SESSIONS, and
  // recent_tabs::USER_SIGNED_OUT falls through to here.
  return OTHER_DEVICES_SECTION;
}

- (CellType)cellType:(NSIndexPath*)indexPath {
  SectionType sectionType = [self sectionType:indexPath.section];
  switch (sectionType) {
    case CLOSED_TAB_SECTION:
      if (indexPath.row == 0) {
        return CELL_CLOSED_TAB_SECTION_HEADER;
      }
      // The last cell of the section is to access the history panel.
      if (indexPath.row ==
          [self numberOfCellsInRecentlyClosedTabsSection] - 1) {
        return CELL_SHOW_FULL_HISTORY;
      }
      return CELL_CLOSED_TAB_DATA;
    case SEPARATOR_SECTION:
      return CELL_SEPARATOR;
    case SESSION_SECTION:
      if (indexPath.row == 0) {
        return CELL_SESSION_SECTION_HEADER;
      }
      return CELL_SESSION_TAB_DATA;
    case OTHER_DEVICES_SECTION:
      if (_sessionState ==
          SessionsSyncUserState::USER_SIGNED_IN_SYNC_IN_PROGRESS) {
        return CELL_OTHER_DEVICES_SYNC_IN_PROGRESS;
      }
      if (indexPath.row == 0) {
        return CELL_OTHER_DEVICES_SECTION_HEADER;
      }
      switch (_sessionState) {
        case SessionsSyncUserState::USER_SIGNED_OUT:
          return CELL_OTHER_DEVICES_SIGNIN_PROMO;
        case SessionsSyncUserState::USER_SIGNED_IN_SYNC_OFF:
          return CELL_OTHER_DEVICES_SIGNED_IN_SYNC_OFF;
        case SessionsSyncUserState::USER_SIGNED_IN_SYNC_ON_NO_SESSIONS:
          return CELL_OTHER_DEVICES_SIGNED_IN_SYNC_ON_NO_SESSIONS;
        case SessionsSyncUserState::USER_SIGNED_IN_SYNC_ON_WITH_SESSIONS:
        case SessionsSyncUserState::USER_SIGNED_IN_SYNC_IN_PROGRESS:
          NOTREACHED();
          // These cases should never occur. Still, this method needs to
          // return _something_, so it's returning the least wrong cell type.
          return CELL_OTHER_DEVICES_SIGNED_IN_SYNC_ON_NO_SESSIONS;
      }
  }
}

- (NSInteger)sectionIndexForSectionType:(SectionType)sectionType {
  NSUInteger sectionCount = [self numberOfSectionsInTableView:self.tableView];
  for (NSUInteger sectionIndex = 0; sectionIndex < sectionCount;
       ++sectionIndex) {
    if ([self sectionType:sectionIndex] == sectionType)
      return sectionIndex;
  }
  return NSNotFound;
}

- (NSInteger)numberOfSectionsBeforeSessionOrOtherDevicesSections {
  // The 2 sections are CLOSED_TAB_SECTION and SEPARATOR_SECTION.
  return 2;
}

- (void)dismissModals {
  [_contextMenuCoordinator stop];
}

#pragma mark - Recently closed tab helpers

- (void)refreshRecentlyClosedTabs {
  [self.tableView reloadData];
}

- (void)setTabRestoreService:(sessions::TabRestoreService*)tabRestoreService {
  _tabRestoreService = tabRestoreService;
}

- (NSInteger)numberOfCellsInRecentlyClosedTabsSection {
  // + 2 because of the section header, and the "Show full history" cell.
  return [self numberOfRecentlyClosedTabs] + 2;
}

- (NSInteger)numberOfRecentlyClosedTabs {
  if (!_tabRestoreService)
    return 0;
  return static_cast<NSInteger>(_tabRestoreService->entries().size());
}

- (const sessions::TabRestoreService::Entry*)tabRestoreEntryAtIndex:
    (NSIndexPath*)indexPath {
  DCHECK_EQ([self sectionType:indexPath.section], CLOSED_TAB_SECTION);
  // "- 1" because of the section header.
  NSInteger index = indexPath.row - 1;
  DCHECK_LE(index, [self numberOfRecentlyClosedTabs]);
  if (!_tabRestoreService)
    return nullptr;

  // Advance the entry iterator to the correct index.
  // Note that std:list<> can only be accessed sequentially, which is
  // suboptimal when using Cocoa table APIs. This list doesn't appear
  // to get very long, so it probably won't matter for perf.
  sessions::TabRestoreService::Entries::const_iterator iter =
      _tabRestoreService->entries().begin();
  std::advance(iter, index);
  CHECK(*iter);
  return iter->get();
}

#pragma mark - Helpers to open tabs, or show the full history view.

- (void)dismissRecentTabsModal {
  [self.handsetCommandHandler dismissRecentTabsWithCompletion:nil];
}

- (void)openTabWithContentOfDistantTab:
    (synced_sessions::DistantTab const*)distantTab {
  sync_sessions::OpenTabsUIDelegate* openTabs =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(_browserState)
          ->GetOpenTabsUIDelegate();
  const sessions::SessionTab* toLoad = nullptr;
  [self dismissRecentTabsModal];
  if (openTabs->GetForeignTab(distantTab->session_tag, distantTab->tab_id,
                              &toLoad)) {
    base::RecordAction(base::UserMetricsAction(
        "MobileRecentTabManagerTabFromOtherDeviceOpened"));
    new_tab_page_uma::RecordAction(
        _browserState, new_tab_page_uma::ACTION_OPENED_FOREIGN_SESSION);
    [_loader loadSessionTab:toLoad];
  }
}

- (void)openTabWithTabRestoreEntry:
    (const sessions::TabRestoreService::Entry*)entry {
  DCHECK(entry);
  if (!entry)
    return;
  // We only handle the TAB type.
  if (entry->type != sessions::TabRestoreService::TAB)
    return;
  TabRestoreServiceDelegateImplIOS* delegate =
      TabRestoreServiceDelegateImplIOSFactory::GetForBrowserState(
          _browserState);
  [self dismissRecentTabsModal];
  base::RecordAction(
      base::UserMetricsAction("MobileRecentTabManagerRecentTabOpened"));
  new_tab_page_uma::RecordAction(
      _browserState, new_tab_page_uma::ACTION_OPENED_RECENTLY_CLOSED_ENTRY);
  _tabRestoreService->RestoreEntryById(delegate, entry->id,
                                       WindowOpenDisposition::CURRENT_TAB);
}

- (void)openTabWithURL:(const GURL&)url {
  if (url.is_valid()) {
    [self dismissRecentTabsModal];

    web::NavigationManager::WebLoadParams params(url);
    params.transition_type = ui::PAGE_TRANSITION_TYPED;
    [_loader loadURLWithParams:params];
  }
}

- (void)showFullHistory {
  __weak LegacyRecentTabsTableViewController* weakSelf = self;
  ProceduralBlock openHistory = ^{
    [weakSelf.dispatcher showHistory];
  };
  DCHECK(self.handsetCommandHandler);
  [self.handsetCommandHandler dismissRecentTabsWithCompletion:openHistory];
}

#pragma mark - Handling of the collapsed sections.

- (void)toggleExpansionOfSection:(NSInteger)sectionIndex {
  NSString* sectionCollapseKey = nil;
  int cellCount = 0;

  SectionType section = [self sectionType:sectionIndex];

  switch (section) {
    case CLOSED_TAB_SECTION:
      sectionCollapseKey = kRecentlyClosedCollapsedKey;
      // - 1 because the header does not count.
      cellCount = [self numberOfCellsInRecentlyClosedTabsSection] - 1;
      break;
    case SEPARATOR_SECTION:
      NOTREACHED();
      return;
    case OTHER_DEVICES_SECTION:
      cellCount = 1;
      sectionCollapseKey = kOtherDeviceCollapsedKey;
      break;
    case SESSION_SECTION: {
      size_t indexOfSession =
          sectionIndex -
          [self numberOfSectionsBeforeSessionOrOtherDevicesSections];
      DCHECK_LT(indexOfSession, _syncedSessions->GetSessionCount());
      synced_sessions::DistantSession const* distantSession =
          _syncedSessions->GetSession(indexOfSession);
      cellCount = distantSession->tabs.size();
      sectionCollapseKey = [self keyForDistantSession:distantSession];
      break;
    }
  }
  DCHECK(sectionCollapseKey);
  BOOL collapsed = ![self sectionIsCollapsed:sectionCollapseKey];
  [self setSection:sectionCollapseKey collapsed:collapsed];

  // Builds an array indexing all the cells needing to be removed or inserted to
  // collapse/expand the section.
  NSMutableArray* cellIndexPathsToDeleteOrInsert = [NSMutableArray array];
  for (int i = 1; i <= cellCount; i++) {
    NSIndexPath* tabIndexPath =
        [NSIndexPath indexPathForRow:i inSection:sectionIndex];
    [cellIndexPathsToDeleteOrInsert addObject:tabIndexPath];
  }

  // Update the table view.
  [self.tableView beginUpdates];
  if (collapsed) {
    [self.tableView deleteRowsAtIndexPaths:cellIndexPathsToDeleteOrInsert
                          withRowAnimation:UITableViewRowAnimationFade];
  } else {
    [self.tableView insertRowsAtIndexPaths:cellIndexPathsToDeleteOrInsert
                          withRowAnimation:UITableViewRowAnimationFade];
  }
  [self.tableView endUpdates];

  // Rotate disclosure icon.
  NSIndexPath* sectionCellIndexPath =
      [NSIndexPath indexPathForRow:0 inSection:sectionIndex];
  UITableViewCell* sectionCell =
      [self.tableView cellForRowAtIndexPath:sectionCellIndexPath];
  UIView* subview = [sectionCell viewWithTag:kSectionHeader];
  DCHECK([subview
      conformsToProtocol:@protocol(HeaderOfCollapsableSectionProtocol)]);
  id<HeaderOfCollapsableSectionProtocol> headerView =
      static_cast<id<HeaderOfCollapsableSectionProtocol>>(subview);
  [headerView setSectionIsCollapsed:collapsed animated:YES];
}

- (NSString*)keyForDistantSession:
    (synced_sessions::DistantSession const*)distantSession {
  return base::SysUTF8ToNSString(distantSession->tag);
}

- (void)setSection:(NSString*)sectionKey collapsed:(BOOL)collapsed {
  // TODO(crbug.com/419346): Store in the browser state preference instead of
  // NSUserDefaults.
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSDictionary* collapsedSections =
      [defaults dictionaryForKey:kCollapsedSectionsKey];
  NSMutableDictionary* newCollapsedSessions =
      [NSMutableDictionary dictionaryWithDictionary:collapsedSections];
  NSNumber* value = [NSNumber numberWithBool:collapsed];
  [newCollapsedSessions setValue:value forKey:sectionKey];
  [defaults setObject:newCollapsedSessions forKey:kCollapsedSectionsKey];
}

- (BOOL)sectionIsCollapsed:(NSString*)sectionKey {
  // TODO(crbug.com/419346): Store in the profile's preference instead of the
  // NSUserDefaults.
  DCHECK(sectionKey);
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSDictionary* collapsedSessions =
      [defaults dictionaryForKey:kCollapsedSectionsKey];
  NSNumber* value = (NSNumber*)[collapsedSessions valueForKey:sectionKey];
  return [value boolValue];
}

#pragma mark - Distant Sessions helpers

- (void)refreshUserState:(SessionsSyncUserState)newSessionState {
  if ((newSessionState == _sessionState &&
       _sessionState !=
           SessionsSyncUserState::USER_SIGNED_IN_SYNC_ON_WITH_SESSIONS) ||
      _signinPromoViewMediator.isSigninInProgress) {
    // No need to refresh the sections.
    return;
  }

  [self.tableView beginUpdates];
  NSIndexSet* indexesToBeDeleted = [self sessionOrOtherDevicesSectionsIndexes];
  [self.tableView deleteSections:indexesToBeDeleted
                withRowAnimation:UITableViewRowAnimationFade];
  syncer::SyncService* syncService =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(_browserState);
  _syncedSessions.reset(new synced_sessions::SyncedSessions(syncService));
  _sessionState = newSessionState;

  if (_sessionState == SessionsSyncUserState::USER_SIGNED_IN_SYNC_IN_PROGRESS) {
    // Expand the "Other Device" section once sync is finished.
    [self setSection:kOtherDeviceCollapsedKey collapsed:NO];
  }

  NSIndexSet* indexesToBeInserted = [self sessionOrOtherDevicesSectionsIndexes];
  [self.tableView insertSections:indexesToBeInserted
                withRowAnimation:UITableViewRowAnimationFade];
  [self.tableView endUpdates];

  if (_sessionState != SessionsSyncUserState::USER_SIGNED_OUT) {
    [_signinPromoViewMediator signinPromoViewRemoved];
    _signinPromoViewMediator = nil;
  }
}

- (NSInteger)numberOfSessionSections {
  DCHECK(_sessionState ==
         SessionsSyncUserState::USER_SIGNED_IN_SYNC_ON_WITH_SESSIONS);
  return _syncedSessions->GetSessionCount();
}

- (NSIndexSet*)sessionOrOtherDevicesSectionsIndexes {
  NSInteger sectionCount = 0;
  switch (_sessionState) {
    case SessionsSyncUserState::USER_SIGNED_IN_SYNC_ON_WITH_SESSIONS:
      sectionCount = [self numberOfSessionSections];
      break;
    case SessionsSyncUserState::USER_SIGNED_IN_SYNC_OFF:
    case SessionsSyncUserState::USER_SIGNED_IN_SYNC_ON_NO_SESSIONS:
    case SessionsSyncUserState::USER_SIGNED_OUT:
    case SessionsSyncUserState::USER_SIGNED_IN_SYNC_IN_PROGRESS:
      sectionCount = 1;
      break;
  }
  NSRange rangeOfSessionSections = NSMakeRange(
      [self numberOfSectionsBeforeSessionOrOtherDevicesSections], sectionCount);
  NSIndexSet* sessionSectionsIndexes =
      [NSIndexSet indexSetWithIndexesInRange:rangeOfSessionSections];
  return sessionSectionsIndexes;
}

- (size_t)indexOfSessionAtIndexPath:(NSIndexPath*)indexPath {
  DCHECK_EQ([self sectionType:indexPath.section], SESSION_SECTION);
  size_t indexOfSession =
      indexPath.section -
      [self numberOfSectionsBeforeSessionOrOtherDevicesSections];
  DCHECK_LT(indexOfSession, _syncedSessions->GetSessionCount());
  return indexOfSession;
}

- (synced_sessions::DistantSession const*)sessionAtIndexPath:
    (NSIndexPath*)indexPath {
  return _syncedSessions->GetSession(
      [self indexOfSessionAtIndexPath:indexPath]);
}

- (synced_sessions::DistantTab const*)distantTabAtIndex:
    (NSIndexPath*)indexPath {
  DCHECK_EQ([self sectionType:indexPath.section], SESSION_SECTION);
  // "- 1" because of the section header.
  size_t indexOfDistantTab = indexPath.row - 1;
  synced_sessions::DistantSession const* session =
      [self sessionAtIndexPath:indexPath];
  DCHECK_LT(indexOfDistantTab, session->tabs.size());
  return session->tabs[indexOfDistantTab].get();
}

#pragma mark - Long press and context menus

- (void)handleLongPress:(UILongPressGestureRecognizer*)longPressGesture {
  DCHECK_EQ(self.tableView, longPressGesture.view);
  if (longPressGesture.state == UIGestureRecognizerStateBegan) {
    CGPoint point = [longPressGesture locationInView:self.tableView];
    NSIndexPath* indexPath = [self.tableView indexPathForRowAtPoint:point];
    if (!indexPath)
      return;
    DCHECK_LE(indexPath.section,
              [self numberOfSectionsInTableView:self.tableView]);

    CellType cellType = [self cellType:indexPath];
    if (cellType != CELL_SESSION_SECTION_HEADER) {
      NOTREACHED();
      return;
    }

    web::ContextMenuParams params;
    // Get view coordinates in local space.
    CGPoint viewCoordinate = [longPressGesture locationInView:self.tableView];
    params.location = viewCoordinate;
    params.view = self.tableView;

    // Present sheet/popover using controller that is added to view hierarchy.
    // TODO(crbug.com/754642): Remove TopPresentedViewController().
    UIViewController* topController =
        top_view_controller::TopPresentedViewController();

    _contextMenuCoordinator =
        [[ContextMenuCoordinator alloc] initWithBaseViewController:topController
                                                            params:params];

    // Fill the sheet/popover with buttons.
    __weak LegacyRecentTabsTableViewController* weakSelf = self;

    // "Open all tabs" button.
    NSString* openAllButtonLabel =
        l10n_util::GetNSString(IDS_IOS_RECENT_TABS_OPEN_ALL_MENU_OPTION);
    [_contextMenuCoordinator
        addItemWithTitle:openAllButtonLabel
                  action:^{
                    [weakSelf openTabsFromSessionAtIndexPath:indexPath];
                  }];

    // "Hide for now" button.
    NSString* hideButtonLabel =
        l10n_util::GetNSString(IDS_IOS_RECENT_TABS_HIDE_MENU_OPTION);
    [_contextMenuCoordinator
        addItemWithTitle:hideButtonLabel
                  action:^{
                    [weakSelf removeSessionAtIndexPath:indexPath];
                  }];

    [_contextMenuCoordinator start];
  }
}

- (void)openTabsFromSessionAtIndexPath:(NSIndexPath*)indexPath {
  synced_sessions::DistantSession const* session =
      [self sessionAtIndexPath:indexPath];
  [self dismissRecentTabsModal];
  for (auto const& tab : session->tabs) {
    [_loader webPageOrderedOpen:tab->virtual_url
                       referrer:web::Referrer()
                   inBackground:YES
                       appendTo:kLastTab];
  }
}

- (void)removeSessionAtIndexPath:(NSIndexPath*)indexPath {
  DCHECK_EQ([self cellType:indexPath], CELL_SESSION_SECTION_HEADER);
  synced_sessions::DistantSession const* session =
      [self sessionAtIndexPath:indexPath];
  std::string sessionTagCopy = session->tag;
  syncer::SyncService* syncService =
      IOSChromeProfileSyncServiceFactory::GetForBrowserState(_browserState);
  sync_sessions::OpenTabsUIDelegate* openTabs =
      syncService->GetOpenTabsUIDelegate();
  _syncedSessions->EraseSession([self indexOfSessionAtIndexPath:indexPath]);
  [self.tableView
        deleteSections:[NSIndexSet indexSetWithIndex:indexPath.section]
      withRowAnimation:UITableViewRowAnimationLeft];
  // Use dispatch_async to give the action sheet a chance to cleanup before
  // replacing its parent view.
  dispatch_async(dispatch_get_main_queue(), ^{
    openTabs->DeleteForeignSession(sessionTagCopy);
  });
}

#pragma mark - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer*)gestureRecognizer {
  CGPoint point = [gestureRecognizer locationInView:self.tableView];
  NSIndexPath* indexPath = [self.tableView indexPathForRowAtPoint:point];
  if (!indexPath)
    return NO;
  CellType cellType = [self cellType:indexPath];
  // Context menus can be opened on a section header for tabs.
  return cellType == CELL_SESSION_SECTION_HEADER;
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView*)tableView {
  switch (_sessionState) {
    case SessionsSyncUserState::USER_SIGNED_IN_SYNC_ON_WITH_SESSIONS:
      return [self numberOfSectionsBeforeSessionOrOtherDevicesSections] +
             [self numberOfSessionSections];
    case SessionsSyncUserState::USER_SIGNED_IN_SYNC_OFF:
    case SessionsSyncUserState::USER_SIGNED_IN_SYNC_ON_NO_SESSIONS:
    case SessionsSyncUserState::USER_SIGNED_OUT:
    case SessionsSyncUserState::USER_SIGNED_IN_SYNC_IN_PROGRESS:
      return [self numberOfSectionsBeforeSessionOrOtherDevicesSections] + 1;
  }
}

- (NSInteger)tableView:(UITableView*)tableView
    numberOfRowsInSection:(NSInteger)section {
  switch ([self sectionType:section]) {
    case CLOSED_TAB_SECTION:
      if ([self sectionIsCollapsed:kRecentlyClosedCollapsedKey])
        return 1;
      else
        return [self numberOfCellsInRecentlyClosedTabsSection];
    case SEPARATOR_SECTION:
      return 1;
    case OTHER_DEVICES_SECTION:
      if (_sessionState ==
          SessionsSyncUserState::USER_SIGNED_IN_SYNC_IN_PROGRESS)
        return 1;
      if ([self sectionIsCollapsed:kOtherDeviceCollapsedKey])
        return 1;
      else
        return 2;
    case SESSION_SECTION: {
      DCHECK(_sessionState ==
             SessionsSyncUserState::USER_SIGNED_IN_SYNC_ON_WITH_SESSIONS);
      size_t sessionIndex =
          section - [self numberOfSectionsBeforeSessionOrOtherDevicesSections];
      DCHECK_LT(sessionIndex, _syncedSessions->GetSessionCount());
      synced_sessions::DistantSession const* distantSession =
          _syncedSessions->GetSession(sessionIndex);
      NSString* key = [self keyForDistantSession:distantSession];
      if ([self sectionIsCollapsed:key])
        return 1;
      else
        return distantSession->tabs.size() + 1;
    }
  }
}

- (UITableViewCell*)tableView:(UITableView*)tableView
        cellForRowAtIndexPath:(NSIndexPath*)indexPath {
  UITableViewCell* cell =
      [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault
                             reuseIdentifier:nil];
  UIView* contentView = cell.contentView;
  CGFloat contentViewTopMargin = 0;

  UIView* subview;
  CellType cellType = [self cellType:indexPath];
  switch (cellType) {
    case CELL_CLOSED_TAB_SECTION_HEADER: {
      BOOL collapsed = [self sectionIsCollapsed:kRecentlyClosedCollapsedKey];
      subview = [[GenericSectionHeaderView alloc]
                initWithType:recent_tabs::RECENTLY_CLOSED_TABS_SECTION_HEADER
          sectionIsCollapsed:collapsed];
      [subview setTag:kSectionHeader];
      break;
    }
    case CELL_CLOSED_TAB_DATA: {
      SessionTabDataView* genericTabData =
          [[SessionTabDataView alloc] initWithFrame:CGRectZero];
      [genericTabData
          updateWithTabRestoreEntry:[self tabRestoreEntryAtIndex:indexPath]
                       browserState:_browserState];
      subview = genericTabData;
      break;
    }
    case CELL_SHOW_FULL_HISTORY:
      subview = [[ShowFullHistoryView alloc] initWithFrame:CGRectZero];
      break;
    case CELL_SEPARATOR:
      subview = [[RecentlyClosedSectionFooter alloc] initWithFrame:CGRectZero];
      [cell setSelectionStyle:UITableViewCellSelectionStyleNone];
      break;
    case CELL_OTHER_DEVICES_SECTION_HEADER: {
      BOOL collapsed = [self sectionIsCollapsed:kOtherDeviceCollapsedKey];
      subview = [[GenericSectionHeaderView alloc]
                initWithType:recent_tabs::OTHER_DEVICES_SECTION_HEADER
          sectionIsCollapsed:collapsed];
      [subview setTag:kSectionHeader];
      break;
    }
    case CELL_OTHER_DEVICES_SIGNED_IN_SYNC_OFF:
      subview = [[SignedInSyncOffView alloc] initWithFrame:CGRectZero
                                              browserState:_browserState
                                                 presenter:self];
      [cell setSelectionStyle:UITableViewCellSelectionStyleNone];
      break;
    case CELL_OTHER_DEVICES_SIGNED_IN_SYNC_ON_NO_SESSIONS:
      subview = [[SignedInSyncOnNoSessionsView alloc] initWithFrame:CGRectZero];
      [cell setSelectionStyle:UITableViewCellSelectionStyleNone];
      break;
    case CELL_OTHER_DEVICES_SIGNIN_PROMO: {
      if (!_signinPromoViewMediator) {
        _signinPromoViewMediator = [[SigninPromoViewMediator alloc]
            initWithBrowserState:_browserState
                     accessPoint:signin_metrics::AccessPoint::
                                     ACCESS_POINT_RECENT_TABS
                       presenter:self /* id<SigninPresenter> */];
        _signinPromoViewMediator.consumer = self;
      }
      contentViewTopMargin = kSigninPromoViewTopMargin;
      SigninPromoView* signinPromoView =
          [[SigninPromoView alloc] initWithFrame:CGRectZero];
      signinPromoView.delegate = _signinPromoViewMediator;
      signinPromoView.textLabel.text =
          l10n_util::GetNSString(IDS_IOS_SIGNIN_PROMO_RECENT_TABS);
      signinPromoView.textLabel.preferredMaxLayoutWidth =
          CGRectGetWidth(self.tableView.bounds) -
          2 * signinPromoView.horizontalPadding;
      SigninPromoViewConfigurator* configurator =
          [_signinPromoViewMediator createConfigurator];
      [configurator configureSigninPromoView:signinPromoView];
      subview = signinPromoView;
      [cell setSelectionStyle:UITableViewCellSelectionStyleNone];
      [_signinPromoViewMediator signinPromoViewVisible];
      break;
    }
    case CELL_OTHER_DEVICES_SYNC_IN_PROGRESS:
      subview = [[SignedInSyncInProgressView alloc] initWithFrame:CGRectZero];
      [cell setSelectionStyle:UITableViewCellSelectionStyleNone];
      break;
    case CELL_SESSION_SECTION_HEADER: {
      synced_sessions::DistantSession const* distantSession =
          [self sessionAtIndexPath:indexPath];
      NSString* key = [self keyForDistantSession:distantSession];
      BOOL collapsed = [self sectionIsCollapsed:key];
      SessionSectionHeaderView* sessionSectionHeader =
          [[SessionSectionHeaderView alloc] initWithFrame:CGRectZero
                                       sectionIsCollapsed:collapsed];
      [sessionSectionHeader updateWithSession:distantSession];
      subview = sessionSectionHeader;
      [subview setTag:kSectionHeader];
      break;
    }
    case CELL_SESSION_TAB_DATA: {
      SessionTabDataView* genericTabData =
          [[SessionTabDataView alloc] initWithFrame:CGRectZero];
      [genericTabData updateWithDistantTab:[self distantTabAtIndex:indexPath]
                              browserState:_browserState];
      subview = genericTabData;
      break;
    }
  }

  DCHECK(subview);
  [contentView addSubview:subview];

  // Sets constraints on the subview.
  [subview setTranslatesAutoresizingMaskIntoConstraints:NO];

  NSDictionary* viewsDictionary = @{@"view" : subview};
  // This set of constraints should match the constraints set on the
  // RecentlyClosedSectionFooter.
  // clang-format off
  NSArray* constraints = @[
    @"V:|-(TopMargin)-[view]-0-|",
    @"H:|-(>=0)-[view(<=548)]-(>=0)-|",
    @"H:[view(==548@500)]"
  ];
  // clang-format on
  [contentView addConstraint:[NSLayoutConstraint
                                 constraintWithItem:subview
                                          attribute:NSLayoutAttributeCenterX
                                          relatedBy:NSLayoutRelationEqual
                                             toItem:contentView
                                          attribute:NSLayoutAttributeCenterX
                                         multiplier:1
                                           constant:0]];
  NSDictionary* metrics = @{ @"TopMargin" : @(contentViewTopMargin) };
  ApplyVisualConstraintsWithMetrics(constraints, viewsDictionary, metrics);
  return cell;
}

#pragma mark - UITableViewDelegate

- (NSIndexPath*)tableView:(UITableView*)tableView
    willSelectRowAtIndexPath:(NSIndexPath*)indexPath {
  DCHECK_EQ(tableView, self.tableView);
  CellType cellType = [self cellType:indexPath];
  switch (cellType) {
    case CELL_CLOSED_TAB_SECTION_HEADER:
    case CELL_OTHER_DEVICES_SECTION_HEADER:
    case CELL_SESSION_SECTION_HEADER:
    case CELL_CLOSED_TAB_DATA:
    case CELL_SESSION_TAB_DATA:
    case CELL_SHOW_FULL_HISTORY:
      return indexPath;
    case CELL_SEPARATOR:
    case CELL_OTHER_DEVICES_SIGNED_IN_SYNC_OFF:
    case CELL_OTHER_DEVICES_SIGNED_IN_SYNC_ON_NO_SESSIONS:
    case CELL_OTHER_DEVICES_SIGNIN_PROMO:
    case CELL_OTHER_DEVICES_SYNC_IN_PROGRESS:
      return nil;
  }
}

- (void)tableView:(UITableView*)tableView
    didSelectRowAtIndexPath:(NSIndexPath*)indexPath {
  DCHECK_EQ(tableView, self.tableView);
  CellType cellType = [self cellType:indexPath];
  switch (cellType) {
    case CELL_CLOSED_TAB_SECTION_HEADER:
    case CELL_OTHER_DEVICES_SECTION_HEADER:
    case CELL_SESSION_SECTION_HEADER:
      // Collapse or uncollapse section.
      [tableView deselectRowAtIndexPath:indexPath animated:NO];
      [self toggleExpansionOfSection:indexPath.section];
      break;
    case CELL_CLOSED_TAB_DATA:
      // Open new tab.
      [self openTabWithTabRestoreEntry:[self tabRestoreEntryAtIndex:indexPath]];
      break;
    case CELL_SESSION_TAB_DATA:
      // Open new tab.
      [self openTabWithContentOfDistantTab:[self distantTabAtIndex:indexPath]];
      break;
    case CELL_SHOW_FULL_HISTORY:
      [tableView deselectRowAtIndexPath:indexPath animated:NO];
      [self showFullHistory];
      break;
    case CELL_SEPARATOR:
    case CELL_OTHER_DEVICES_SIGNED_IN_SYNC_OFF:
    case CELL_OTHER_DEVICES_SIGNED_IN_SYNC_ON_NO_SESSIONS:
    case CELL_OTHER_DEVICES_SIGNIN_PROMO:
    case CELL_OTHER_DEVICES_SYNC_IN_PROGRESS:
      NOTREACHED();
      break;
  }
}

- (CGFloat)tableView:(UITableView*)tableView
    heightForRowAtIndexPath:(NSIndexPath*)indexPath {
  DCHECK_EQ(self.tableView, tableView);
  CellType cellType = [self cellType:indexPath];
  switch (cellType) {
    case CELL_SHOW_FULL_HISTORY:
      return [ShowFullHistoryView desiredHeightInUITableViewCell];
    case CELL_SEPARATOR:
      return [RecentlyClosedSectionFooter desiredHeightInUITableViewCell];
    case CELL_OTHER_DEVICES_SIGNED_IN_SYNC_OFF:
      return [SignedInSyncOffView desiredHeightInUITableViewCell];
    case CELL_OTHER_DEVICES_SIGNED_IN_SYNC_ON_NO_SESSIONS:
      return [SignedInSyncOnNoSessionsView desiredHeightInUITableViewCell];
    case CELL_OTHER_DEVICES_SIGNIN_PROMO:
      return UITableViewAutomaticDimension;
    case CELL_SESSION_SECTION_HEADER:
      return [SessionSectionHeaderView desiredHeightInUITableViewCell];
    case CELL_CLOSED_TAB_DATA:
    case CELL_SESSION_TAB_DATA:
      return [SessionTabDataView desiredHeightInUITableViewCell];
    case CELL_CLOSED_TAB_SECTION_HEADER:
    case CELL_OTHER_DEVICES_SECTION_HEADER:
      return [GenericSectionHeaderView desiredHeightInUITableViewCell];
    case CELL_OTHER_DEVICES_SYNC_IN_PROGRESS:
      return [SignedInSyncInProgressView desiredHeightInUITableViewCell];
  }
}

- (UIView*)tableView:(UITableView*)tableView
    viewForHeaderInSection:(NSInteger)section {
  if ([self sectionType:section] == CLOSED_TAB_SECTION) {
    return [[RecentlyTabsTopSpacingHeader alloc] initWithFrame:CGRectZero];
  }
  return nil;
}

- (CGFloat)tableView:(UITableView*)tableView
    heightForHeaderInSection:(NSInteger)section {
  if ([self sectionType:section] == CLOSED_TAB_SECTION) {
    return [RecentlyTabsTopSpacingHeader desiredHeightInUITableViewCell];
  }
  return 0;
}

#pragma mark - SigninPromoViewConsumer

- (void)configureSigninPromoWithConfigurator:
            (SigninPromoViewConfigurator*)configurator
                             identityChanged:(BOOL)identityChanged {
  DCHECK(_signinPromoViewMediator);
  if ([self sectionIsCollapsed:kOtherDeviceCollapsedKey])
    return;
  NSInteger sectionIndex =
      [self sectionIndexForSectionType:OTHER_DEVICES_SECTION];
  DCHECK(sectionIndex != NSNotFound);
  NSIndexPath* indexPath =
      [NSIndexPath indexPathForRow:1 inSection:sectionIndex];
  if (identityChanged) {
    [self.tableView reloadRowsAtIndexPaths:@[ indexPath ]
                          withRowAnimation:UITableViewRowAnimationNone];
    return;
  }
  UITableViewCell* cell = [self.tableView cellForRowAtIndexPath:indexPath];
  NSArray<UIView*>* contentViews = cell.contentView.subviews;
  DCHECK(contentViews.count == 1);
  UIView* subview = contentViews[0];
  DCHECK([subview isKindOfClass:[SigninPromoView class]]);
  SigninPromoView* signinPromoView = (SigninPromoView*)subview;
  [configurator configureSigninPromoView:signinPromoView];
}

- (void)signinDidFinish {
  [self.delegate refreshSessionsView];
}

#pragma mark - SyncPresenter

- (void)showReauthenticateSignin {
  [self.dispatcher
              showSignin:
                  [[ShowSigninCommand alloc]
                      initWithOperation:AUTHENTICATION_OPERATION_REAUTHENTICATE
                            accessPoint:signin_metrics::AccessPoint::
                                            ACCESS_POINT_UNKNOWN]
      baseViewController:self];
}

- (void)showSyncSettings {
  [self.dispatcher showSyncSettingsFromViewController:self];
}

- (void)showSyncPassphraseSettings {
  [self.dispatcher showSyncPassphraseSettingsFromViewController:self];
}

#pragma mark - SigninPresenter

- (void)showSignin:(ShowSigninCommand*)command {
  [self.dispatcher showSignin:command baseViewController:self];
}

@end

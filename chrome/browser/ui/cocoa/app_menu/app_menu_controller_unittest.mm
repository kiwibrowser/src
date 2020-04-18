// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/app_menu/app_menu_controller.h"
#include "base/command_line.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/sync/profile_sync_test_util.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_bridge.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_cocoa_controller.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/browser/ui/cocoa/test/run_loop_testing.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#import "chrome/browser/ui/cocoa/view_resizer_pong.h"
#include "chrome/browser/ui/sync/browser_synced_window_delegates_getter.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "chrome/browser/ui/toolbar/recent_tabs_builder_test_helper.h"
#include "chrome/browser/ui/toolbar/recent_tabs_sub_menu_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/browser_sync/profile_sync_service_mock.h"
#include "components/sync/base/sync_prefs.h"
#include "components/sync/device_info/local_device_info_provider_mock.h"
#include "components/sync/driver/sync_client.h"
#include "components/sync/model/fake_sync_change_processor.h"
#include "components/sync/model/sync_error_factory_mock.h"
#include "components/sync_sessions/sessions_sync_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

using testing::_;
using testing::Invoke;
using testing::Return;

@implementation AppMenuController (TestingAPI)
- (BookmarkMenuBridge*)bookmarkMenuBridge {
  return bookmarkMenuBridge_.get();
}
@end

@interface TestBookmarkMenuCocoaController : BookmarkMenuCocoaController
@property(nonatomic, readonly) const bookmarks::BookmarkNode* lastOpenedNode;
@end

@implementation TestBookmarkMenuCocoaController {
  const bookmarks::BookmarkNode* lastOpenedNode_;
}
@synthesize lastOpenedNode = lastOpenedNode_;
- (void)openURLForNode:(const bookmarks::BookmarkNode*)node {
  lastOpenedNode_ = node;
}
@end

namespace {

class MockAppMenuModel : public AppMenuModel {
 public:
  MockAppMenuModel() : AppMenuModel(nullptr, nullptr) {}
  ~MockAppMenuModel() {}
  MOCK_METHOD2(ExecuteCommand, void(int command_id, int event_flags));
};

class BrowserRemovedObserver : public BrowserListObserver {
 public:
  BrowserRemovedObserver() { BrowserList::AddObserver(this); }
  ~BrowserRemovedObserver() override { BrowserList::RemoveObserver(this); }
  void WaitUntilBrowserRemoved() { run_loop_.Run(); }
  void OnBrowserRemoved(Browser* browser) override { run_loop_.Quit(); }

 private:
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(BrowserRemovedObserver);
};

}  // namespace

namespace test {

class AppMenuControllerTest : public CocoaProfileTest {
 public:
  AppMenuControllerTest() {
    TestingProfile::TestingFactories factories;
    factories.push_back(std::make_pair(ProfileSyncServiceFactory::GetInstance(),
                                       BuildMockProfileSyncService));
    AddTestingFactories(factories);
  }

  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    local_device_ = std::make_unique<syncer::LocalDeviceInfoProviderMock>(
        "AppMenuControllerTest", "Test Machine", "Chromium 10k", "Chrome 10k",
        sync_pb::SyncEnums_DeviceType_TYPE_LINUX, "device_id");

    controller_.reset([[AppMenuController alloc] initWithBrowser:browser()]);

    fake_model_ = std::make_unique<MockAppMenuModel>();

    sync_prefs_ = std::make_unique<syncer::SyncPrefs>(profile()->GetPrefs());

    mock_sync_service_ = static_cast<browser_sync::ProfileSyncServiceMock*>(
        ProfileSyncServiceFactory::GetInstance()->GetForProfile(profile()));

    manager_ = std::make_unique<sync_sessions::SessionsSyncManager>(
        ProfileSyncServiceFactory::GetForProfile(profile())
            ->GetSyncClient()
            ->GetSyncSessionsClient(),
        sync_prefs_.get(), local_device_.get(), base::Closure());

    manager_->MergeDataAndStartSyncing(
        syncer::SESSIONS, syncer::SyncDataList(),
        std::unique_ptr<syncer::SyncChangeProcessor>(
            new syncer::FakeSyncChangeProcessor),
        std::unique_ptr<syncer::SyncErrorFactory>(
            new syncer::SyncErrorFactoryMock));
  }

  TestBookmarkMenuCocoaController* SetTestBookmarksMenuDelegate(
      BookmarkMenuBridge* bridge) {
    base::scoped_nsobject<TestBookmarkMenuCocoaController> test_controller(
        [[TestBookmarkMenuCocoaController alloc] initWithBridge:bridge]);
    bridge->ClearBookmarkMenu();
    [bridge->BookmarkMenu() setDelegate:test_controller];
    bridge->controller_ =
        base::scoped_nsobject<BookmarkMenuCocoaController>(test_controller);
    return test_controller;
  }

  void RegisterRecentTabs(RecentTabsBuilderTestHelper* helper) {
    helper->ExportToSessionsSyncManager(manager_.get());
  }

  void TearDown() override {
    fake_model_.reset();
    controller_.reset();
    manager_.reset();
    sync_prefs_.reset();
    local_device_.reset();
    CocoaProfileTest::TearDown();
  }

  void EnableSync() {
    EXPECT_CALL(*mock_sync_service_, IsSyncActive())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_sync_service_,
                IsDataTypeControllerRunning(syncer::SESSIONS))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_sync_service_,
                IsDataTypeControllerRunning(syncer::PROXY_TABS))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_sync_service_, GetOpenTabsUIDelegateMock())
        .WillRepeatedly(Return(manager_->GetOpenTabsUIDelegate()));
  }

  AppMenuController* controller() {
    return controller_.get();
  }

  base::scoped_nsobject<AppMenuController> controller_;

  std::unique_ptr<MockAppMenuModel> fake_model_;

 private:
  std::unique_ptr<syncer::LocalDeviceInfoProviderMock> local_device_;
  std::unique_ptr<syncer::SyncPrefs> sync_prefs_;
  browser_sync::ProfileSyncServiceMock* mock_sync_service_ = nullptr;
  std::unique_ptr<sync_sessions::SessionsSyncManager> manager_;
};

TEST_F(AppMenuControllerTest, Initialized) {
  EXPECT_TRUE([controller() menu]);
  EXPECT_GE([[controller() menu] numberOfItems], 5);
}

TEST_F(AppMenuControllerTest, DispatchSimple) {
  base::scoped_nsobject<NSButton> button([[NSButton alloc] init]);
  [button setTag:IDC_ZOOM_PLUS];

  // Set fake model to test dispatching.
  EXPECT_CALL(*fake_model_, ExecuteCommand(IDC_ZOOM_PLUS, 0));
  [controller() setModel:fake_model_.get()];

  [controller() dispatchAppMenuCommand:button.get()];
  chrome::testing::NSRunLoopRunAllPending();
}

TEST_F(AppMenuControllerTest, RecentTabsFavIcon) {
  EnableSync();

  RecentTabsBuilderTestHelper recent_tabs_builder;
  recent_tabs_builder.AddSession();
  recent_tabs_builder.AddWindow(0);
  recent_tabs_builder.AddTab(0, 0);
  RegisterRecentTabs(&recent_tabs_builder);

  RecentTabsSubMenuModel recent_tabs_sub_menu_model(nullptr, browser());
  fake_model_->AddSubMenuWithStringId(
      IDC_RECENT_TABS_MENU, IDS_RECENT_TABS_MENU,
      &recent_tabs_sub_menu_model);

  [controller() setModel:fake_model_.get()];
  NSMenu* menu = [controller() menu];
  [controller() updateRecentTabsSubmenu];

  NSString* title = l10n_util::GetNSStringWithFixup(IDS_RECENT_TABS_MENU);
  NSMenu* recent_tabs_menu = [[menu itemWithTitle:title] submenu];
  EXPECT_TRUE(recent_tabs_menu);
  EXPECT_EQ(6, [recent_tabs_menu numberOfItems]);

  // Send a icon changed event and verify that the icon is updated.
  gfx::Image icon(ui::ResourceBundle::GetSharedInstance().GetNativeImageNamed(
      IDR_BOOKMARKS_FAVICON));
  recent_tabs_sub_menu_model.SetIcon(3, icon);
  EXPECT_NSNE(icon.ToNSImage(), [[recent_tabs_menu itemAtIndex:3] image]);
  recent_tabs_sub_menu_model.GetMenuModelDelegate()->OnIconChanged(3);
  EXPECT_TRUE([[recent_tabs_menu itemAtIndex:3] image]);
  EXPECT_NSEQ(icon.ToNSImage(), [[recent_tabs_menu itemAtIndex:3] image]);

  controller_.reset();
  fake_model_.reset();
}

TEST_F(AppMenuControllerTest, RecentTabsElideTitle) {
  EnableSync();

  // Add 1 session with 1 window and 2 tabs.
  RecentTabsBuilderTestHelper recent_tabs_builder;
  recent_tabs_builder.AddSession();
  recent_tabs_builder.AddWindow(0);
  base::string16 tab1_short_title = base::ASCIIToUTF16("Short");
  recent_tabs_builder.AddTabWithInfo(0, 0, base::Time::Now(), tab1_short_title);
  base::string16 tab2_long_title = base::ASCIIToUTF16(
      "Very very very very very very very very very very very very long");
  recent_tabs_builder.AddTabWithInfo(0, 0,
      base::Time::Now() - base::TimeDelta::FromMinutes(10), tab2_long_title);
  RegisterRecentTabs(&recent_tabs_builder);

  RecentTabsSubMenuModel recent_tabs_sub_menu_model(nullptr, browser());
  fake_model_->AddSubMenuWithStringId(
      IDC_RECENT_TABS_MENU, IDS_RECENT_TABS_MENU,
      &recent_tabs_sub_menu_model);

  [controller() setModel:fake_model_.get()];
  NSMenu* menu = [controller() menu];
  [controller() updateRecentTabsSubmenu];

  NSString* title = l10n_util::GetNSStringWithFixup(IDS_RECENT_TABS_MENU);
  NSMenu* recent_tabs_menu = [[menu itemWithTitle:title] submenu];
  EXPECT_TRUE(recent_tabs_menu);
  EXPECT_EQ(7, [recent_tabs_menu numberOfItems]);

  // Item 1: separator.
  EXPECT_TRUE([[recent_tabs_menu itemAtIndex:1] isSeparatorItem]);

  // Index 2: restore tabs menu item.
  NSString* restore_tab_label = l10n_util::FixUpWindowsStyleLabel(
      recent_tabs_sub_menu_model.GetLabelAt(2));
  EXPECT_NSEQ(restore_tab_label, [[recent_tabs_menu itemAtIndex:2] title]);

  // Item 3: separator.
  EXPECT_TRUE([[recent_tabs_menu itemAtIndex:3] isSeparatorItem]);

  // Item 4: window title.
  EXPECT_NSEQ(
      base::SysUTF16ToNSString(recent_tabs_sub_menu_model.GetLabelAt(4)),
      [[recent_tabs_menu itemAtIndex:4] title]);

  // Item 5: short tab title.
  EXPECT_NSEQ(base::SysUTF16ToNSString(tab1_short_title),
              [[recent_tabs_menu itemAtIndex:5] title]);

  // Item 6: long tab title.
  NSString* tab2_actual_title = [[recent_tabs_menu itemAtIndex:6] title];
  NSUInteger title_length = [tab2_actual_title length];
  EXPECT_GT(tab2_long_title.size(), title_length);
  NSString* actual_substring =
      [tab2_actual_title substringToIndex:title_length - 1];
  NSString* expected_substring = [base::SysUTF16ToNSString(tab2_long_title)
      substringToIndex:title_length - 1];
  EXPECT_NSEQ(expected_substring, actual_substring);

  controller_.reset();
  fake_model_.reset();
}

// Verify that |RecentTabsMenuModelDelegate| is deleted before the model
// it's observing.
TEST_F(AppMenuControllerTest, RecentTabDeleteOrder) {
  [controller_ menuNeedsUpdate:[controller_ menu]];
  // If the delete order is wrong then the test will crash on exit.
}

// Simulate opening a bookmark from the bookmark submenu.
TEST_F(AppMenuControllerTest, OpenBookmark) {
  // Ensure there's at least one bookmark.
  bookmarks::BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(profile());
  ASSERT_TRUE(model);
  const bookmarks::BookmarkNode* parent = model->bookmark_bar_node();
  const bookmarks::BookmarkNode* about = model->AddURL(
      parent, 0, base::ASCIIToUTF16("About"), GURL("chrome://chrome"));

  EXPECT_FALSE([controller_ bookmarkMenuBridge]);

  [controller_ menuNeedsUpdate:[controller_ menu]];
  BookmarkMenuBridge* bridge = [controller_ bookmarkMenuBridge];
  EXPECT_TRUE(bridge);

  NSMenu* bookmarks_menu =
      [[[controller_ menu] itemWithTitle:@"Bookmarks"] submenu];
  EXPECT_TRUE(bookmarks_menu);
  EXPECT_TRUE([bookmarks_menu delegate]);

  // The bookmark item actions would do Browser::OpenURL(), which will fail in a
  // unit test. So swap in a test delegate.
  TestBookmarkMenuCocoaController* test_controller =
      SetTestBookmarksMenuDelegate(bridge);

  // The fixed items from the default model.
  EXPECT_EQ(5u, [bookmarks_menu numberOfItems]);

  // When AppKit shows the menu, the bookmark items are added (and a separator).
  [[bookmarks_menu delegate] menuNeedsUpdate:bookmarks_menu];
  EXPECT_EQ(7u, [bookmarks_menu numberOfItems]);

  base::scoped_nsobject<NSMenuItem> item(
      [[bookmarks_menu itemWithTitle:@"About"] retain]);
  EXPECT_TRUE(item);
  EXPECT_TRUE([item target]);
  EXPECT_TRUE([item action]);

  // Simulate how AppKit would click the item (menuDidClose happens first).
  [controller_ menuDidClose:[controller_ menu]];
  EXPECT_FALSE([test_controller lastOpenedNode]);
  [[item target] performSelector:[item action] withObject:item];
  EXPECT_EQ(about, [test_controller lastOpenedNode]);
}

// Test that AppMenuController can be destroyed after the Browser.
// This can happen because the AppMenuController's owner (ToolbarController)
// can outlive the Browser.
TEST_F(AppMenuControllerTest, DestroyedAfterBrowser) {
  BrowserRemovedObserver observer;
  // This is normally called by ToolbarController, but since |controller_| is
  // not owned by one, call it here.
  [controller_ browserWillBeDestroyed];
  CloseBrowserWindow();
  observer.WaitUntilBrowserRemoved();
  // |controller_| is released in TearDown().
}

}  // namespace test

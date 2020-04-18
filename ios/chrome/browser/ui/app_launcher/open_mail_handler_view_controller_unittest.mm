// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/app_launcher/open_mail_handler_view_controller.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#import "ios/chrome/browser/web/mailto_handler.h"
#import "ios/chrome/browser/web/mailto_handler_manager.h"
#include "ios/chrome/grit/ios_strings.h"
#include "testing/gtest_mac.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class OpenMailHandlerViewControllerTest : public CollectionViewControllerTest {
 protected:
  CollectionViewController* InstantiateController() override {
    return [[OpenMailHandlerViewController alloc]
        initWithManager:manager_
        selectedHandler:selected_handler_];
  }

  // Returns an OCMock object representing a MailtoHandler object with name
  // |app_name| and ID of |app_id|. The app will be listed with availability of
  // |available|. Returns as id so further expect/stub can be done. Caller
  // should cast it to the proper type if necessary.
  id CreateMockApp(NSString* app_name, NSString* app_id, BOOL available) {
    id mail_app = OCMClassMock([MailtoHandler class]);
    OCMStub([mail_app appName]).andReturn(app_name);
    OCMStub([mail_app appStoreID]).andReturn(app_id);
    OCMStub([mail_app isAvailable]).andReturn(available);
    return mail_app;
  }

  MailtoHandlerManager* manager_;
  OpenMailtoHandlerSelectedHandler selected_handler_;
};

// Verifies the basic structure of the model.
TEST_F(OpenMailHandlerViewControllerTest, TestModel) {
  // Mock manager_ must be created before the controller.
  id manager_mock = OCMClassMock([MailtoHandlerManager class]);
  NSArray* mail_apps = @[
    CreateMockApp(@"app1", @"111", YES), CreateMockApp(@"app2", @"222", NO),
    CreateMockApp(@"app3", @"333", YES)
  ];
  OCMStub([manager_mock defaultHandlers]).andReturn(mail_apps);
  manager_ = base::mac::ObjCCastStrict<MailtoHandlerManager>(manager_mock);
  CreateController();
  CheckController();

  // Verifies the number of sections and items within each section.
  EXPECT_EQ(3, NumberOfSections());
  // Title
  EXPECT_EQ(1, NumberOfItemsInSection(0));
  // Apps. Only 2 out of 3 mailto:// handlers are actually availabel.
  EXPECT_EQ(2, NumberOfItemsInSection(1));
  // Footer with toggle option.
  EXPECT_EQ(1, NumberOfItemsInSection(2));
  CheckSwitchCellStateAndTitleWithId(YES, IDS_IOS_CHOOSE_EMAIL_ASK_TOGGLE, 2,
                                     0);
}

// Verifies that when the second row is selected, the callback is called with
// the second mailto:// handler.
TEST_F(OpenMailHandlerViewControllerTest, TestSelectionCallback) {
  // Mock manager_ and callback block must be created before the controller.
  id manager_mock = OCMClassMock([MailtoHandlerManager class]);
  NSArray* mailApps = @[
    CreateMockApp(@"app1", @"111", YES), CreateMockApp(@"app2", @"222", YES)
  ];
  OCMStub([manager_mock defaultHandlers]).andReturn(mailApps);
  manager_ = base::mac::ObjCCastStrict<MailtoHandlerManager>(manager_mock);
  __block BOOL handler_called = NO;
  selected_handler_ = ^(MailtoHandler* handler) {
    EXPECT_NSEQ(@"222", [handler appStoreID]);
    handler_called = YES;
  };
  CreateController();
  OpenMailHandlerViewController* test_view_controller =
      base::mac::ObjCCastStrict<OpenMailHandlerViewController>(controller());

  // Tests that when the second app which has an ID of @"222" is selected, the
  // callback block is called with the correct |handler|.
  NSIndexPath* selected_index_path =
      [NSIndexPath indexPathForRow:1 inSection:1];
  [test_view_controller collectionView:[test_view_controller collectionView]
              didSelectItemAtIndexPath:selected_index_path];
  EXPECT_TRUE(handler_called);
}

// Verifies that if the "always ask" toggle is turned OFF and a certain row
// in the view controller is selected, the corresponding mailto:// handler
// app is set as the default in |manager_|.
TEST_F(OpenMailHandlerViewControllerTest, TestAlwaysAskToggle) {
  // Mock manager_ must be created before the controller.
  id manager_mock = OCMClassMock([MailtoHandlerManager class]);
  NSArray* mail_apps = @[
    CreateMockApp(@"app1", @"111", YES), CreateMockApp(@"app2", @"222", YES)
  ];
  OCMStub([manager_mock defaultHandlers]).andReturn(mail_apps);
  // The second app will be selected for this test.
  OCMExpect([manager_mock setDefaultHandlerID:@"222"]);
  manager_ = base::mac::ObjCCastStrict<MailtoHandlerManager>(manager_mock);
  CreateController();

  // Finds the UISwitch cell and toggle the switch to OFF.
  OpenMailHandlerViewController* test_view_controller =
      base::mac::ObjCCastStrict<OpenMailHandlerViewController>(controller());
  NSIndexPath* switch_index_path = [NSIndexPath indexPathForRow:0 inSection:2];
  CollectionViewSwitchCell* switch_cell =
      base::mac::ObjCCastStrict<CollectionViewSwitchCell>([test_view_controller
                  collectionView:[test_view_controller collectionView]
          cellForItemAtIndexPath:switch_index_path]);
  switch_cell.switchView.on = NO;
  [switch_cell.switchView
      sendActionsForControlEvents:UIControlEventValueChanged];

  // Then selects the second app in the list of apps.
  NSIndexPath* selected_index_path =
      [NSIndexPath indexPathForRow:1 inSection:1];
  [test_view_controller collectionView:[test_view_controller collectionView]
              didSelectItemAtIndexPath:selected_index_path];
  EXPECT_OCMOCK_VERIFY(manager_mock);
}

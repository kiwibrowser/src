// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sync_internals_ui.h"

#include <cstddef>
#include <string>

#include "base/message_loop/message_loop_current.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/browser_sync/profile_sync_service_mock.h"
#include "components/sync/js/js_arg_list.h"
#include "components/sync/js/js_event_details.h"
#include "components/sync/js/js_test_util.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/test/test_browser_thread.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;

// Rewrite to use WebUI testing infrastructure. Current code below is mostly
// testing how WebUI concrete class serializes function parameters, and that
// SyncInternalsUI::HandleJSEvent/HandleJsReply prefix the given function with
// "chrome.sync." and postfix it with ".fire" or ".handleReply".
// http://crbug.com/110517
// Also, instead of using ProfileMock, call SetTestingFactoryAndUse on the
// PasswordStoreFactory to set up a particular store during testing.
/*

namespace {

using browser_sync::HasArgsAsList;
using syncer::JsArgList;
using syncer::JsEventDetails;
using content::BrowserThread;
using content::WebContents;
using testing::_;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::StrictMock;

// Subclass of WebUI to mock out ExecuteJavascript.
class TestSyncWebUI: public WebUI {
 public:
  explicit TestSyncWebUI(WebContents* web_contents)
      : WebUI(web_contents) {}
  virtual ~TestSyncWebUI() {}

  MOCK_METHOD1(ExecuteJavascript, void(const base::string16&));
};

// Tests with non-NULL ProfileSyncService.
class SyncInternalsUITestWithService : public ChromeRenderViewHostTestHarness {
 protected:
  SyncInternalsUITestWithService() : sync_internals_ui_(NULL) {}

  virtual ~SyncInternalsUITestWithService() {}

  virtual void SetUp() {
    NiceMock<ProfileMock>* profile_mock = new NiceMock<ProfileMock>();
    StrictMock<browser_sync::ProfileSyncServiceMock> profile_sync_service_mock;
    EXPECT_CALL(*profile_mock, GetProfileSyncService())
        .WillOnce(Return(&profile_sync_service_mock));
    browser_context_.reset(profile_mock);

    ChromeRenderViewHostTestHarness::SetUp();

    EXPECT_CALL(profile_sync_service_mock, GetJsController())
        .WillOnce(Return(mock_js_controller_.AsWeakPtr()));

    EXPECT_CALL(mock_js_controller_, AddJsEventHandler(_));

    // |sync_internals_ui_|'s constructor triggers all the
    // expectations above.
    web_ui_.reset(new TestSyncWebUI(web_contents()));
    sync_internals_ui_ = new SyncInternalsUI(web_ui_.get());
    web_ui_->SetController(sync_internals_ui_);

    Mock::VerifyAndClearExpectations(profile_mock);
    Mock::VerifyAndClearExpectations(&mock_js_controller_);
  }

  virtual void TearDown() {
    Mock::VerifyAndClearExpectations(&mock_js_controller_);

    // Called by |sync_internals_ui_|'s destructor.
    EXPECT_CALL(mock_js_controller_,
                RemoveJsEventHandler(sync_internals_ui_));
    sync_internals_ui_ = NULL;
    web_ui_.reset();

    ChromeRenderViewHostTestHarness::TearDown();
  }

  StrictMock<browser_sync::MockJsController> mock_js_controller_;
  std::unique_ptr<TestSyncWebUI> web_ui_;
  SyncInternalsUI* sync_internals_ui_;
};

TEST_F(SyncInternalsUITestWithService, HandleJsEvent) {
  EXPECT_CALL(*web_ui_,
              ExecuteJavascript(
                  ASCIIToUTF16("chrome.sync.testMessage.fire({});")));

  sync_internals_ui_->HandleJsEvent("testMessage", JsEventDetails());
}

TEST_F(SyncInternalsUITestWithService, HandleJsReply) {
  EXPECT_CALL(
      *web_ui_,
      ExecuteJavascript(
          ASCIIToUTF16("chrome.sync.testMessage.handleReply(5,true);")));

  base::ListValue args;
  args.Append(new base::Value(5));
  args.Append(new base::Value(true));
  sync_internals_ui_->HandleJsReply("testMessage", JsArgList(&args));
}

TEST_F(SyncInternalsUITestWithService, OnWebUISendBasic) {
  const std::string& name = "testName";
  base::ListValue args;
  args.Append(new base::Value(10));

  EXPECT_CALL(mock_js_controller_,
              ProcessJsMessage(name, HasArgsAsList(args), _));

  sync_internals_ui_->OverrideHandleWebUIMessage(GURL(), name, args);
}

// Tests with NULL ProfileSyncService.
class SyncInternalsUITestWithoutService
    : public ChromeRenderViewHostTestHarness {
 protected:
  SyncInternalsUITestWithoutService() : sync_internals_ui_(NULL) {}

  virtual ~SyncInternalsUITestWithoutService() {}

  virtual void SetUp() {
    NiceMock<ProfileMock>* profile_mock = new NiceMock<ProfileMock>();
    EXPECT_CALL(*profile_mock, GetProfileSyncService())
        .WillOnce(Return(static_cast<browser_sync::ProfileSyncService*>(NULL)));
    browser_context_.reset(profile_mock);

    ChromeRenderViewHostTestHarness::SetUp();

    // |sync_internals_ui_|'s constructor triggers all the
    // expectations above.
    web_ui_.reset(new TestSyncWebUI(web_contents()));
    sync_internals_ui_ = new SyncInternalsUI(web_ui_.get());
    web_ui_->SetController(sync_internals_ui_);

    Mock::VerifyAndClearExpectations(profile_mock);
  }

  std::unique_ptr<TestSyncWebUI> web_ui_;
  SyncInternalsUI* sync_internals_ui_;
};

TEST_F(SyncInternalsUITestWithoutService, HandleJsEvent) {
  EXPECT_CALL(*web_ui_,
              ExecuteJavascript(
                  ASCIIToUTF16("chrome.sync.testMessage.fire({});")));

  sync_internals_ui_->HandleJsEvent("testMessage", JsEventDetails());
}

TEST_F(SyncInternalsUITestWithoutService, HandleJsReply) {
  EXPECT_CALL(
      *web_ui_,
      ExecuteJavascript(
          ASCIIToUTF16("chrome.sync.testMessage.handleReply(5,true);")));

  base::ListValue args;
  args.Append(new base::Value(5));
  args.Append(new base::Value(true));
  sync_internals_ui_->HandleJsReply(
      "testMessage", JsArgList(&args));
}

TEST_F(SyncInternalsUITestWithoutService, OnWebUISendBasic) {
  const std::string& name = "testName";
  base::ListValue args;
  args.Append(new base::Value(5));

  // Should drop the message.
  sync_internals_ui_->OverrideHandleWebUIMessage(GURL(), name, args);
}

// TODO(lipalani) - add a test case to test about:sync with a non null
// service.
TEST_F(SyncInternalsUITestWithoutService, OnWebUISendGetAboutInfo) {
  const char kAboutInfoCall[] =
      "chrome.sync.getAboutInfo.handleReply({\"summary\":\"SYNC DISABLED\"});";
  EXPECT_CALL(*web_ui_,
              ExecuteJavascript(ASCIIToUTF16(kAboutInfoCall)));

  base::ListValue args;
  sync_internals_ui_->OverrideHandleWebUIMessage(
      GURL(), "getAboutInfo", args);
}

}  // namespace

*/

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/macros.h"
#include "chrome/browser/media/router/test/test_helper.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/toolbar/mock_media_router_action_controller.h"
#include "chrome/browser/ui/webui/media_router/media_router_dialog_controller_webui_impl.h"
#include "chrome/browser/ui/webui/media_router/media_router_ui.h"
#include "chrome/browser/ui/webui/media_router/media_router_ui_service.h"
#include "chrome/browser/ui/webui/media_router/media_router_web_ui_test.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"

using blink::mojom::PresentationInfo;
using content::WebContents;

namespace media_router {

class MediaRouterDialogControllerWebUIImplTest : public MediaRouterWebUITest {
 public:
  MediaRouterDialogControllerWebUIImplTest() : MediaRouterWebUITest(true) {}
  ~MediaRouterDialogControllerWebUIImplTest() override {}

  void OpenMediaRouterDialog();

  MOCK_METHOD2(PresentationSuccessCallback,
               void(const blink::mojom::PresentationInfo&, const MediaRoute&));
  MOCK_METHOD1(PresentationErrorCallback,
               void(const blink::mojom::PresentationError& error));

 protected:
  WebContents* initiator_ = nullptr;
  MediaRouterDialogControllerWebUIImpl* dialog_controller_ = nullptr;
  WebContents* media_router_dialog_ = nullptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaRouterDialogControllerWebUIImplTest);
};

void MediaRouterDialogControllerWebUIImplTest::OpenMediaRouterDialog() {
  // Start with one window with one tab.
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  EXPECT_EQ(0, browser()->tab_strip_model()->count());
  chrome::NewTab(browser());
  EXPECT_EQ(1, browser()->tab_strip_model()->count());

  // Create a reference to initiator contents.
  initiator_ = browser()->tab_strip_model()->GetActiveWebContents();

  dialog_controller_ =
      MediaRouterDialogControllerWebUIImpl::GetOrCreateForWebContents(
          initiator_);
  ASSERT_TRUE(dialog_controller_);

  // Get the media router dialog for the initiator.
  dialog_controller_->ShowMediaRouterDialog();
  media_router_dialog_ = dialog_controller_->GetMediaRouterDialog();
  ASSERT_TRUE(media_router_dialog_);

  // New media router dialog is a constrained window, so the number of
  // tabs is still 1.
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_NE(initiator_, media_router_dialog_);
  EXPECT_EQ(media_router_dialog_, dialog_controller_->GetMediaRouterDialog());
}

// Create/Get a media router dialog for initiator.
TEST_F(MediaRouterDialogControllerWebUIImplTest, ShowMediaRouterDialog) {
  OpenMediaRouterDialog();

  // Show media router dialog for the same initiator again.
  dialog_controller_->ShowMediaRouterDialog();
  WebContents* same_media_router_dialog =
      dialog_controller_->GetMediaRouterDialog();

  // Tab count remains the same.
  EXPECT_EQ(1, browser()->tab_strip_model()->count());

  // Media router dialog already exists. Calling |ShowMediaRouterDialog| again
  // should not have created a new media router dialog.
  EXPECT_EQ(media_router_dialog_, same_media_router_dialog);
}

// Tests multiple media router dialogs exist in the same browser for different
// initiators. If a dialog already exists for an initiator, that initiator
// gets focused.
TEST_F(MediaRouterDialogControllerWebUIImplTest, MultipleMediaRouterDialogs) {
  // Let's start with one window and two tabs.
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  TabStripModel* tab_strip_model = browser()->tab_strip_model();
  ASSERT_TRUE(tab_strip_model);

  EXPECT_EQ(0, tab_strip_model->count());

  // Create some new initiators.
  chrome::NewTab(browser());
  WebContents* web_contents_1 = tab_strip_model->GetActiveWebContents();
  ASSERT_TRUE(web_contents_1);

  chrome::NewTab(browser());
  WebContents* web_contents_2 = tab_strip_model->GetActiveWebContents();
  ASSERT_TRUE(web_contents_2);
  EXPECT_EQ(2, tab_strip_model->count());

  // Create media router dialog for |web_contents_1|.
  MediaRouterDialogControllerWebUIImpl* dialog_controller_1 =
      MediaRouterDialogControllerWebUIImpl::GetOrCreateForWebContents(
          web_contents_1);
  ASSERT_TRUE(dialog_controller_1);

  dialog_controller_1->ShowMediaRouterDialog();
  WebContents* media_router_dialog_1 =
      dialog_controller_1->GetMediaRouterDialog();

  ASSERT_TRUE(media_router_dialog_1);

  EXPECT_NE(web_contents_1, media_router_dialog_1);
  EXPECT_EQ(2, tab_strip_model->count());

  // Create media router dialog for |web_contents_2|.
  MediaRouterDialogControllerWebUIImpl* dialog_controller_2 =
      MediaRouterDialogControllerWebUIImpl::GetOrCreateForWebContents(
          web_contents_2);
  ASSERT_TRUE(dialog_controller_2);

  dialog_controller_2->ShowMediaRouterDialog();
  WebContents* media_router_dialog_2 =
      dialog_controller_2->GetMediaRouterDialog();
  ASSERT_TRUE(media_router_dialog_2);

  EXPECT_NE(web_contents_2, media_router_dialog_2);
  EXPECT_NE(media_router_dialog_1, media_router_dialog_2);

  // 2 initiators and 2 dialogs exist in the same browser. The dialogs are
  // constrained in their respective initiators.
  EXPECT_EQ(2, tab_strip_model->count());

  int tab_1_index = tab_strip_model->GetIndexOfWebContents(web_contents_1);
  int tab_2_index = tab_strip_model->GetIndexOfWebContents(web_contents_2);
  int media_router_dialog_1_index =
      tab_strip_model->GetIndexOfWebContents(media_router_dialog_1);
  int media_router_dialog_2_index =
      tab_strip_model->GetIndexOfWebContents(media_router_dialog_2);

  // Constrained dialogs are not in the TabStripModel.
  EXPECT_EQ(-1, media_router_dialog_1_index);
  EXPECT_EQ(-1, media_router_dialog_2_index);

  // Since |media_router_dialog_2_index| was the most recently created dialog,
  // its initiator should have focus.
  EXPECT_EQ(tab_2_index, tab_strip_model->active_index());

  // When we get the media router dialog for |web_contents_1|,
  // |media_router_dialog_1| is activated and focused.
  dialog_controller_1->ShowMediaRouterDialog();
  EXPECT_EQ(tab_1_index, tab_strip_model->active_index());

  // When we get the media router dialog for |web_contents_2|,
  // |media_router_dialog_2| is activated and focused.
  dialog_controller_2->ShowMediaRouterDialog();
  EXPECT_EQ(tab_2_index, tab_strip_model->active_index());
}

TEST_F(MediaRouterDialogControllerWebUIImplTest, CloseDialogFromWebUI) {
  OpenMediaRouterDialog();

  // Close the dialog.
  content::WebContentsDestroyedWatcher watcher(media_router_dialog_);

  content::WebUI* web_ui = media_router_dialog_->GetWebUI();
  ASSERT_TRUE(web_ui);
  MediaRouterUI* media_router_ui =
      static_cast<MediaRouterUI*>(web_ui->GetController());
  ASSERT_TRUE(media_router_ui);
  media_router_ui->Close();

  // Blocks until the media router dialog WebContents has been destroyed.
  watcher.Wait();

  // Still 1 tab in the browser.
  EXPECT_EQ(1, browser()->tab_strip_model()->count());

  // Entry has been removed.
  EXPECT_FALSE(dialog_controller_->GetMediaRouterDialog());

  // Show the media router dialog again, creating a new one.
  dialog_controller_->ShowMediaRouterDialog();
  WebContents* media_router_dialog_2 =
      dialog_controller_->GetMediaRouterDialog();

  // Still 1 tab in the browser.
  EXPECT_EQ(1, browser()->tab_strip_model()->count());

  EXPECT_EQ(media_router_dialog_2, dialog_controller_->GetMediaRouterDialog());
}

TEST_F(MediaRouterDialogControllerWebUIImplTest,
       CloseDialogFromDialogController) {
  OpenMediaRouterDialog();

  // Close the dialog.
  content::WebContentsDestroyedWatcher watcher(media_router_dialog_);

  dialog_controller_->HideMediaRouterDialog();

  // Blocks until the media router dialog WebContents has been destroyed.
  watcher.Wait();

  // Still 1 tab in the browser.
  EXPECT_EQ(1, browser()->tab_strip_model()->count());

  // Entry has been removed.
  EXPECT_FALSE(dialog_controller_->GetMediaRouterDialog());
}

TEST_F(MediaRouterDialogControllerWebUIImplTest, CloseInitiator) {
  OpenMediaRouterDialog();

  // Close the initiator. This should also close the dialog WebContents.
  content::WebContentsDestroyedWatcher initiator_watcher(initiator_);
  content::WebContentsDestroyedWatcher dialog_watcher(media_router_dialog_);

  int initiator_index =
      browser()->tab_strip_model()->GetIndexOfWebContents(initiator_);
  EXPECT_NE(-1, initiator_index);
  EXPECT_TRUE(browser()->tab_strip_model()->CloseWebContentsAt(
      initiator_index, TabStripModel::CLOSE_NONE));

  // Blocks until the initiator WebContents has been destroyed.
  initiator_watcher.Wait();
  dialog_watcher.Wait();

  // No tabs in the browser.
  EXPECT_EQ(0, browser()->tab_strip_model()->count());

  // The dialog controller is deleted when the WebContents is closed/destroyed.
}

TEST_F(MediaRouterDialogControllerWebUIImplTest, NotifyActionController) {
  MockMediaRouterActionController* action_controller =
      static_cast<MockMediaRouterActionController*>(
          MediaRouterUIService::Get(browser()->profile())->action_controller());
  ASSERT_TRUE(action_controller);

  EXPECT_CALL(*action_controller, OnDialogShown());
  OpenMediaRouterDialog();
  EXPECT_CALL(*action_controller, OnDialogHidden());
  dialog_controller_->HideMediaRouterDialog();

  EXPECT_CALL(*action_controller, OnDialogShown());
  EXPECT_TRUE(dialog_controller_->ShowMediaRouterDialogForPresentation(
      content::PresentationRequest(
          {1, 2}, {GURL("http://test.com"), GURL("http://test2.com")},
          url::Origin::Create(GURL("http://example.com"))),
      base::BindOnce(&MediaRouterDialogControllerWebUIImplTest::
                         PresentationSuccessCallback,
                     base::Unretained(this)),
      base::BindOnce(
          &MediaRouterDialogControllerWebUIImplTest::PresentationErrorCallback,
          base::Unretained(this))));

  // When |dialog_controller_| is destroyed with its dialog open,
  // |action_controller| should be notified.
  EXPECT_CALL(*action_controller, OnDialogHidden());
  EXPECT_CALL(*this, PresentationErrorCallback(testing::_));
}

}  // namespace media_router

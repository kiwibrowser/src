// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "chrome/browser/media/router/test/mock_media_router.h"
#include "chrome/browser/ui/toolbar/component_action_delegate.h"
#include "chrome/browser/ui/toolbar/component_toolbar_actions_factory.h"
#include "chrome/browser/ui/toolbar/media_router_action_controller.h"
#include "chrome/browser/ui/webui/media_router/media_router_web_ui_test.h"
#include "chrome/common/pref_names.h"
#include "testing/gmock/include/gmock/gmock.h"

class FakeComponentActionDelegate : public ComponentActionDelegate {
 public:
  FakeComponentActionDelegate() {}
  ~FakeComponentActionDelegate() override {}

  void AddComponentAction(const std::string& action_id) override {
    EXPECT_EQ(action_id, ComponentToolbarActionsFactory::kMediaRouterActionId);
    EXPECT_FALSE(HasComponentAction(
        ComponentToolbarActionsFactory::kMediaRouterActionId));
    has_media_router_action_ = true;
  }

  void RemoveComponentAction(const std::string& action_id) override {
    EXPECT_EQ(action_id, ComponentToolbarActionsFactory::kMediaRouterActionId);
    EXPECT_TRUE(HasComponentAction(
        ComponentToolbarActionsFactory::kMediaRouterActionId));
    has_media_router_action_ = false;
  }

  bool HasComponentAction(const std::string& action_id) const override {
    EXPECT_EQ(action_id, ComponentToolbarActionsFactory::kMediaRouterActionId);
    return has_media_router_action_;
  }

 private:
  bool has_media_router_action_ = false;
};

class MediaRouterActionControllerUnitTest : public MediaRouterWebUITest {
 public:
  MediaRouterActionControllerUnitTest()
      : issue_(media_router::IssueInfo(
            "title notification",
            media_router::IssueInfo::Action::DISMISS,
            media_router::IssueInfo::Severity::NOTIFICATION)),
        source1_("fakeSource1"),
        source2_("fakeSource2") {}

  ~MediaRouterActionControllerUnitTest() override {}

  // MediaRouterWebUITest:
  void SetUp() override {
    MediaRouterWebUITest::SetUp();

    router_ = std::make_unique<media_router::MockMediaRouter>();
    component_action_delegate_ =
        std::make_unique<FakeComponentActionDelegate>();
    controller_ = std::make_unique<MediaRouterActionController>(
        profile(), router_.get(), component_action_delegate_.get());

    SetAlwaysShowActionPref(false);

    local_display_route_list_.push_back(media_router::MediaRoute(
        "routeId1", source1_, "sinkId1", "description", true, true));
    non_local_display_route_list_.push_back(media_router::MediaRoute(
        "routeId2", source1_, "sinkId2", "description", false, true));
    non_local_display_route_list_.push_back(media_router::MediaRoute(
        "routeId3", source2_, "sinkId3", "description", true, false));
  }

  void TearDown() override {
    controller_.reset();
    component_action_delegate_.reset();
    router_.reset();
    MediaRouterWebUITest::TearDown();
  }

  bool ActionExists() {
    return component_action_delegate_->HasComponentAction(
        ComponentToolbarActionsFactory::kMediaRouterActionId);
  }

  void SetAlwaysShowActionPref(bool always_show) {
    MediaRouterActionController::SetAlwaysShowActionPref(profile(),
                                                         always_show);
  }

  MediaRouterActionController* controller() { return controller_.get(); }

  const media_router::Issue& issue() { return issue_; }
  const std::vector<media_router::MediaRoute>& local_display_route_list()
      const {
    return local_display_route_list_;
  }
  const std::vector<media_router::MediaRoute>& non_local_display_route_list()
      const {
    return non_local_display_route_list_;
  }
  const std::vector<media_router::MediaRoute::Id>& empty_route_id_list() const {
    return empty_route_id_list_;
  }

 private:
  std::unique_ptr<MediaRouterActionController> controller_;
  std::unique_ptr<media_router::MockMediaRouter> router_;
  std::unique_ptr<FakeComponentActionDelegate> component_action_delegate_;

  const media_router::Issue issue_;

  // Fake Sources, used for the Routes.
  const media_router::MediaSource source1_;
  const media_router::MediaSource source2_;

  std::vector<media_router::MediaRoute> local_display_route_list_;
  std::vector<media_router::MediaRoute> non_local_display_route_list_;
  std::vector<media_router::MediaRoute::Id> empty_route_id_list_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterActionControllerUnitTest);
};

TEST_F(MediaRouterActionControllerUnitTest, EphemeralIconForRoutesAndIssues) {
  EXPECT_FALSE(ActionExists());

  // Creating a local route should show the action icon.
  controller()->OnRoutesUpdated(local_display_route_list(),
                                empty_route_id_list());
  EXPECT_TRUE(controller()->has_local_display_route_);
  EXPECT_TRUE(ActionExists());
  // Removing the local route should hide the icon.
  controller()->OnRoutesUpdated(non_local_display_route_list(),
                                empty_route_id_list());
  EXPECT_FALSE(controller()->has_local_display_route_);
  EXPECT_FALSE(ActionExists());

  // Creating an issue should show the action icon.
  controller()->OnIssue(issue());
  EXPECT_TRUE(controller()->has_issue_);
  EXPECT_TRUE(ActionExists());
  // Removing the issue should hide the icon.
  controller()->OnIssuesCleared();
  EXPECT_FALSE(controller()->has_issue_);
  EXPECT_FALSE(ActionExists());

  controller()->OnIssue(issue());
  controller()->OnRoutesUpdated(local_display_route_list(),
                                empty_route_id_list());
  controller()->OnIssuesCleared();
  // When the issue disappears, the icon should remain visible if there's
  // a local route.
  EXPECT_TRUE(ActionExists());
  controller()->OnRoutesUpdated(std::vector<media_router::MediaRoute>(),
                                empty_route_id_list());
  EXPECT_FALSE(ActionExists());
}

TEST_F(MediaRouterActionControllerUnitTest, EphemeralIconForDialog) {
  EXPECT_FALSE(ActionExists());

  // Showing a dialog should show the icon.
  controller()->OnDialogShown();
  EXPECT_TRUE(ActionExists());
  // Showing and hiding a dialog shouldn't hide the icon as long as we have a
  // positive number of dialogs.
  controller()->OnDialogShown();
  EXPECT_TRUE(ActionExists());
  controller()->OnDialogHidden();
  EXPECT_TRUE(ActionExists());
  // When we have zero dialogs, the icon should be hidden.
  controller()->OnDialogHidden();
  EXPECT_FALSE(ActionExists());

  controller()->OnDialogShown();
  EXPECT_TRUE(ActionExists());
  controller()->OnRoutesUpdated(local_display_route_list(),
                                empty_route_id_list());
  // Hiding the dialog while there are local routes shouldn't hide the icon.
  controller()->OnDialogHidden();
  EXPECT_TRUE(ActionExists());
  controller()->OnRoutesUpdated(non_local_display_route_list(),
                                empty_route_id_list());
  EXPECT_FALSE(ActionExists());

  controller()->OnDialogShown();
  EXPECT_TRUE(ActionExists());
  controller()->OnIssue(issue());
  // Hiding the dialog while there is an issue shouldn't hide the icon.
  controller()->OnDialogHidden();
  EXPECT_TRUE(ActionExists());
  controller()->OnIssuesCleared();
  EXPECT_FALSE(ActionExists());
}

TEST_F(MediaRouterActionControllerUnitTest, ObserveAlwaysShowPrefChange) {
  EXPECT_FALSE(ActionExists());

  SetAlwaysShowActionPref(true);
  EXPECT_TRUE(ActionExists());

  controller()->OnRoutesUpdated(local_display_route_list(),
                                empty_route_id_list());
  SetAlwaysShowActionPref(false);
  // Unchecking the option while having a local route shouldn't hide the icon.
  EXPECT_TRUE(ActionExists());

  SetAlwaysShowActionPref(true);
  controller()->OnRoutesUpdated(non_local_display_route_list(),
                                empty_route_id_list());
  // Removing the local route should not hide the icon.
  EXPECT_TRUE(ActionExists());

  SetAlwaysShowActionPref(false);
  EXPECT_FALSE(ActionExists());
}

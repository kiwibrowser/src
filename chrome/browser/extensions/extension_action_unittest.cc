// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_action.h"

#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace extensions {

namespace {

std::unique_ptr<ExtensionAction> CreateAction(ActionInfo::Type type,
                                              const ActionInfo& action_info) {
  scoped_refptr<const Extension> extension = ExtensionBuilder("Test").Build();
  return std::make_unique<ExtensionAction>(*extension, type, action_info);
}

}  // namespace

TEST(ExtensionActionTest, Title) {
  ActionInfo action_info;
  action_info.default_title = "Initial Title";
  std::unique_ptr<ExtensionAction> action =
      CreateAction(ActionInfo::TYPE_PAGE, action_info);

  ASSERT_EQ("Initial Title", action->GetTitle(1));
  action->SetTitle(ExtensionAction::kDefaultTabId, "foo");
  ASSERT_EQ("foo", action->GetTitle(1));
  ASSERT_EQ("foo", action->GetTitle(100));
  action->SetTitle(100, "bar");
  ASSERT_EQ("foo", action->GetTitle(1));
  ASSERT_EQ("bar", action->GetTitle(100));
  action->SetTitle(ExtensionAction::kDefaultTabId, "baz");
  ASSERT_EQ("baz", action->GetTitle(1));
  action->ClearAllValuesForTab(100);
  ASSERT_EQ("baz", action->GetTitle(100));
}

TEST(ExtensionActionTest, Visibility) {
  std::unique_ptr<ExtensionAction> action =
      CreateAction(ActionInfo::TYPE_PAGE, ActionInfo());

  ASSERT_FALSE(action->GetIsVisible(1));
  action->SetIsVisible(ExtensionAction::kDefaultTabId, true);
  ASSERT_TRUE(action->GetIsVisible(1));
  ASSERT_TRUE(action->GetIsVisible(100));

  action->SetIsVisible(ExtensionAction::kDefaultTabId, false);
  ASSERT_FALSE(action->GetIsVisible(1));
  ASSERT_FALSE(action->GetIsVisible(100));
  action->SetIsVisible(100, true);
  ASSERT_FALSE(action->GetIsVisible(1));
  ASSERT_TRUE(action->GetIsVisible(100));

  action->ClearAllValuesForTab(100);
  ASSERT_FALSE(action->GetIsVisible(1));
  ASSERT_FALSE(action->GetIsVisible(100));

  std::unique_ptr<ExtensionAction> browser_action =
      CreateAction(ActionInfo::TYPE_BROWSER, ActionInfo());
  ASSERT_TRUE(browser_action->GetIsVisible(1));
}

TEST(ExtensionActionTest, Icon) {
  ActionInfo action_info;
  action_info.default_icon.Add(16, "icon16.png");
  std::unique_ptr<ExtensionAction> page_action =
      CreateAction(ActionInfo::TYPE_PAGE, action_info);
  ASSERT_TRUE(page_action->default_icon());
  EXPECT_EQ("icon16.png",
            page_action->default_icon()->Get(
                16, ExtensionIconSet::MATCH_EXACTLY));
  EXPECT_EQ("",
            page_action->default_icon()->Get(
                17, ExtensionIconSet::MATCH_BIGGER));
}

TEST(ExtensionActionTest, Badge) {
  std::unique_ptr<ExtensionAction> action =
      CreateAction(ActionInfo::TYPE_PAGE, ActionInfo());
  ASSERT_EQ("", action->GetBadgeText(1));
  action->SetBadgeText(ExtensionAction::kDefaultTabId, "foo");
  ASSERT_EQ("foo", action->GetBadgeText(1));
  ASSERT_EQ("foo", action->GetBadgeText(100));
  action->SetBadgeText(100, "bar");
  ASSERT_EQ("foo", action->GetBadgeText(1));
  ASSERT_EQ("bar", action->GetBadgeText(100));
  action->SetBadgeText(ExtensionAction::kDefaultTabId, "baz");
  ASSERT_EQ("baz", action->GetBadgeText(1));
  action->ClearAllValuesForTab(100);
  ASSERT_EQ("baz", action->GetBadgeText(100));
}

TEST(ExtensionActionTest, BadgeTextColor) {
  std::unique_ptr<ExtensionAction> action =
      CreateAction(ActionInfo::TYPE_PAGE, ActionInfo());
  ASSERT_EQ(0x00000000u, action->GetBadgeTextColor(1));
  action->SetBadgeTextColor(ExtensionAction::kDefaultTabId, 0xFFFF0000u);
  ASSERT_EQ(0xFFFF0000u, action->GetBadgeTextColor(1));
  ASSERT_EQ(0xFFFF0000u, action->GetBadgeTextColor(100));
  action->SetBadgeTextColor(100, 0xFF00FF00);
  ASSERT_EQ(0xFFFF0000u, action->GetBadgeTextColor(1));
  ASSERT_EQ(0xFF00FF00u, action->GetBadgeTextColor(100));
  action->SetBadgeTextColor(ExtensionAction::kDefaultTabId, 0xFF0000FFu);
  ASSERT_EQ(0xFF0000FFu, action->GetBadgeTextColor(1));
  action->ClearAllValuesForTab(100);
  ASSERT_EQ(0xFF0000FFu, action->GetBadgeTextColor(100));
}

TEST(ExtensionActionTest, BadgeBackgroundColor) {
  std::unique_ptr<ExtensionAction> action =
      CreateAction(ActionInfo::TYPE_PAGE, ActionInfo());
  ASSERT_EQ(0x00000000u, action->GetBadgeBackgroundColor(1));
  action->SetBadgeBackgroundColor(ExtensionAction::kDefaultTabId,
                                 0xFFFF0000u);
  ASSERT_EQ(0xFFFF0000u, action->GetBadgeBackgroundColor(1));
  ASSERT_EQ(0xFFFF0000u, action->GetBadgeBackgroundColor(100));
  action->SetBadgeBackgroundColor(100, 0xFF00FF00);
  ASSERT_EQ(0xFFFF0000u, action->GetBadgeBackgroundColor(1));
  ASSERT_EQ(0xFF00FF00u, action->GetBadgeBackgroundColor(100));
  action->SetBadgeBackgroundColor(ExtensionAction::kDefaultTabId,
                                 0xFF0000FFu);
  ASSERT_EQ(0xFF0000FFu, action->GetBadgeBackgroundColor(1));
  action->ClearAllValuesForTab(100);
  ASSERT_EQ(0xFF0000FFu, action->GetBadgeBackgroundColor(100));
}

TEST(ExtensionActionTest, PopupUrl) {
  GURL url_unset;
  GURL url_foo("http://www.example.com/foo.html");
  GURL url_bar("http://www.example.com/bar.html");
  GURL url_baz("http://www.example.com/baz.html");

  ActionInfo action_info;
  action_info.default_popup_url = url_foo;
  std::unique_ptr<ExtensionAction> action =
      CreateAction(ActionInfo::TYPE_PAGE, action_info);

  ASSERT_EQ(url_foo, action->GetPopupUrl(1));
  ASSERT_EQ(url_foo, action->GetPopupUrl(100));
  ASSERT_TRUE(action->HasPopup(1));
  ASSERT_TRUE(action->HasPopup(100));

  action->SetPopupUrl(ExtensionAction::kDefaultTabId, url_unset);
  ASSERT_EQ(url_unset, action->GetPopupUrl(1));
  ASSERT_EQ(url_unset, action->GetPopupUrl(100));
  ASSERT_FALSE(action->HasPopup(1));
  ASSERT_FALSE(action->HasPopup(100));

  action->SetPopupUrl(100, url_bar);
  ASSERT_EQ(url_unset, action->GetPopupUrl(1));
  ASSERT_EQ(url_bar, action->GetPopupUrl(100));

  action->SetPopupUrl(ExtensionAction::kDefaultTabId, url_baz);
  ASSERT_EQ(url_baz, action->GetPopupUrl(1));
  ASSERT_EQ(url_bar, action->GetPopupUrl(100));

  action->ClearAllValuesForTab(100);
  ASSERT_EQ(url_baz, action->GetPopupUrl(1));
  ASSERT_EQ(url_baz, action->GetPopupUrl(100));
}

}  // namespace extensions

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/one_shot_accessibility_tree_search.h"

#include <memory>

#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "content/browser/accessibility/browser_accessibility.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#ifdef OS_ANDROID
#include "content/browser/accessibility/browser_accessibility_manager_android.h"
#endif
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

#ifdef OS_ANDROID
class TestBrowserAccessibilityManager
    : public BrowserAccessibilityManagerAndroid {
 public:
  TestBrowserAccessibilityManager(const ui::AXTreeUpdate& initial_tree)
      : BrowserAccessibilityManagerAndroid(initial_tree, nullptr, nullptr) {}
};
#else
class TestBrowserAccessibilityManager : public BrowserAccessibilityManager {
 public:
  TestBrowserAccessibilityManager(
      const ui::AXTreeUpdate& initial_tree)
      : BrowserAccessibilityManager(initial_tree,
                                    nullptr,
                                    new BrowserAccessibilityFactory()) {}
};
#endif

}  // namespace

// These tests prevent other tests from being run. crbug.com/514632
#if defined(ANDROID) && defined(ADDRESS_SANITIZER)
#define MAYBE_OneShotAccessibilityTreeSearchTest DISABLED_OneShotAccessibilityTreeSearchTets
#else
#define MAYBE_OneShotAccessibilityTreeSearchTest OneShotAccessibilityTreeSearchTest
#endif
class MAYBE_OneShotAccessibilityTreeSearchTest : public testing::Test {
 public:
  MAYBE_OneShotAccessibilityTreeSearchTest() {}
  ~MAYBE_OneShotAccessibilityTreeSearchTest() override {}

 protected:
  void SetUp() override;

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<BrowserAccessibilityManager> tree_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MAYBE_OneShotAccessibilityTreeSearchTest);
};

void MAYBE_OneShotAccessibilityTreeSearchTest::SetUp() {
  ui::AXNodeData root;
  root.id = 1;
  root.SetName("Document");
  root.role = ax::mojom::Role::kRootWebArea;
  root.location = gfx::RectF(0, 0, 800, 600);
  root.AddBoolAttribute(ax::mojom::BoolAttribute::kClipsChildren, true);
  root.child_ids.push_back(2);
  root.child_ids.push_back(3);
  root.child_ids.push_back(6);

  ui::AXNodeData heading;
  heading.id = 2;
  heading.SetName("Heading");
  heading.role = ax::mojom::Role::kHeading;
  heading.location = gfx::RectF(0, 0, 800, 50);

  ui::AXNodeData list;
  list.id = 3;
  list.role = ax::mojom::Role::kList;
  list.location = gfx::RectF(0, 50, 500, 500);
  list.child_ids.push_back(4);
  list.child_ids.push_back(5);

  ui::AXNodeData list_item_1;
  list_item_1.id = 4;
  list_item_1.SetName("Autobots");
  list_item_1.role = ax::mojom::Role::kListItem;
  list_item_1.location = gfx::RectF(10, 10, 200, 30);

  ui::AXNodeData list_item_2;
  list_item_2.id = 5;
  list_item_2.SetName("Decepticons");
  list_item_2.role = ax::mojom::Role::kListItem;
  list_item_2.location = gfx::RectF(10, 40, 200, 60);

  ui::AXNodeData footer;
  footer.id = 6;
  footer.SetName("Footer");
  footer.role = ax::mojom::Role::kFooter;
  footer.location = gfx::RectF(0, 650, 100, 800);

  tree_.reset(new TestBrowserAccessibilityManager(
      MakeAXTreeUpdate(root, heading, list, list_item_1, list_item_2, footer)));
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, GetAll) {
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  ASSERT_EQ(6U, search.CountMatches());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, NoCycle) {
  // If you set a result limit of 1, you won't get the root node back as
  // the first match.
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  search.SetResultLimit(1);
  ASSERT_EQ(1U, search.CountMatches());
  EXPECT_NE(1, search.GetMatchAtIndex(0)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, ForwardsWithStartNode) {
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  search.SetStartNode(tree_->GetFromID(4));
  ASSERT_EQ(2U, search.CountMatches());
  EXPECT_EQ(5, search.GetMatchAtIndex(0)->GetId());
  EXPECT_EQ(6, search.GetMatchAtIndex(1)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, BackwardsWithStartNode) {
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  search.SetStartNode(tree_->GetFromID(4));
  search.SetDirection(OneShotAccessibilityTreeSearch::BACKWARDS);
  ASSERT_EQ(3U, search.CountMatches());
  EXPECT_EQ(3, search.GetMatchAtIndex(0)->GetId());
  EXPECT_EQ(2, search.GetMatchAtIndex(1)->GetId());
  EXPECT_EQ(1, search.GetMatchAtIndex(2)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest,
       BackwardsWithStartNodeForAndroid) {
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  search.SetStartNode(tree_->GetFromID(4));
  search.SetDirection(OneShotAccessibilityTreeSearch::BACKWARDS);
  search.SetResultLimit(3);
  search.SetCanWrapToLastElement(true);
  ASSERT_EQ(3U, search.CountMatches());
  EXPECT_EQ(3, search.GetMatchAtIndex(0)->GetId());
  EXPECT_EQ(2, search.GetMatchAtIndex(1)->GetId());
  EXPECT_EQ(1, search.GetMatchAtIndex(2)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest,
       ForwardsWithStartNodeAndScope) {
  OneShotAccessibilityTreeSearch search(tree_->GetFromID(4));
  search.SetStartNode(tree_->GetFromID(5));
  ASSERT_EQ(1U, search.CountMatches());
  EXPECT_EQ(6, search.GetMatchAtIndex(0)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, ResultLimitZero) {
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  search.SetResultLimit(0);
  ASSERT_EQ(0U, search.CountMatches());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, ResultLimitFive) {
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  search.SetResultLimit(5);
  ASSERT_EQ(5U, search.CountMatches());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, DescendantsOnlyOfRoot) {
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  search.SetStartNode(tree_->GetFromID(1));
  search.SetImmediateDescendantsOnly(true);
  ASSERT_EQ(3U, search.CountMatches());
  EXPECT_EQ(2, search.GetMatchAtIndex(0)->GetId());
  EXPECT_EQ(3, search.GetMatchAtIndex(1)->GetId());
  EXPECT_EQ(6, search.GetMatchAtIndex(2)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, DescendantsOnlyOfNode) {
  OneShotAccessibilityTreeSearch search(tree_->GetFromID(3));
  search.SetImmediateDescendantsOnly(true);
  ASSERT_EQ(2U, search.CountMatches());
  EXPECT_EQ(4, search.GetMatchAtIndex(0)->GetId());
  EXPECT_EQ(5, search.GetMatchAtIndex(1)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest,
       DescendantsOnlyOfNodeWithStartNode) {
  OneShotAccessibilityTreeSearch search(tree_->GetFromID(3));
  search.SetStartNode(tree_->GetFromID(4));
  search.SetImmediateDescendantsOnly(true);
  ASSERT_EQ(1U, search.CountMatches());
  EXPECT_EQ(5, search.GetMatchAtIndex(0)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest,
       DescendantsOnlyOfNodeWithStartNode2) {
  OneShotAccessibilityTreeSearch search(tree_->GetFromID(3));
  search.SetStartNode(tree_->GetFromID(5));
  search.SetImmediateDescendantsOnly(true);
  ASSERT_EQ(0U, search.CountMatches());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest,
       DescendantsOnlyOfNodeWithStartNodeBackwards) {
  OneShotAccessibilityTreeSearch search(tree_->GetFromID(3));
  search.SetStartNode(tree_->GetFromID(5));
  search.SetImmediateDescendantsOnly(true);
  search.SetDirection(OneShotAccessibilityTreeSearch::BACKWARDS);
  ASSERT_EQ(1U, search.CountMatches());
  EXPECT_EQ(4, search.GetMatchAtIndex(0)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, VisibleOnly) {
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  search.SetVisibleOnly(true);
  ASSERT_EQ(5U, search.CountMatches());
  EXPECT_EQ(1, search.GetMatchAtIndex(0)->GetId());
  EXPECT_EQ(2, search.GetMatchAtIndex(1)->GetId());
  EXPECT_EQ(3, search.GetMatchAtIndex(2)->GetId());
  EXPECT_EQ(4, search.GetMatchAtIndex(3)->GetId());
  EXPECT_EQ(5, search.GetMatchAtIndex(4)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, CaseInsensitiveStringMatch) {
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  search.SetSearchText("eCEptiCOn");
  ASSERT_EQ(1U, search.CountMatches());
  EXPECT_EQ(5, search.GetMatchAtIndex(0)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, OnePredicate) {
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  search.AddPredicate(
      [](BrowserAccessibility* start, BrowserAccessibility* current) {
        return current->GetRole() == ax::mojom::Role::kListItem;
      });
  ASSERT_EQ(2U, search.CountMatches());
  EXPECT_EQ(4, search.GetMatchAtIndex(0)->GetId());
  EXPECT_EQ(5, search.GetMatchAtIndex(1)->GetId());
}

TEST_F(MAYBE_OneShotAccessibilityTreeSearchTest, TwoPredicates) {
  OneShotAccessibilityTreeSearch search(tree_->GetRoot());
  search.AddPredicate(
      [](BrowserAccessibility* start, BrowserAccessibility* current) {
        return (current->GetRole() == ax::mojom::Role::kList ||
                current->GetRole() == ax::mojom::Role::kListItem);
      });
  search.AddPredicate([](BrowserAccessibility* start,
                         BrowserAccessibility* current) {
    return (current->GetId() % 2 == 1);
  });
  ASSERT_EQ(2U, search.CountMatches());
  EXPECT_EQ(3, search.GetMatchAtIndex(0)->GetId());
  EXPECT_EQ(5, search.GetMatchAtIndex(1)->GetId());
}

}  // namespace content

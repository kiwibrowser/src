// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by node BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/paint_property_node.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/geometry/float_rounded_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_paint_property_node.h"
#include "third_party/blink/renderer/platform/testing/paint_property_test_helpers.h"

namespace blink {

class PaintPropertyNodeTest : public testing::Test {
 protected:
  void SetUp() override {
    root = ClipPaintPropertyNode::Root();
    node = CreateClip(root, nullptr, FloatRoundedRect());
    child1 = CreateClip(node, nullptr, FloatRoundedRect());
    child2 = CreateClip(node, nullptr, FloatRoundedRect());
    grandchild1 = CreateClip(child1, nullptr, FloatRoundedRect());
    grandchild2 = CreateClip(child2, nullptr, FloatRoundedRect());

    //          root
    //           |
    //          node
    //         /   \
    //     child1   child2
    //       |        |
    // grandchild1 grandchild2
  }

  void ResetAllChanged() {
    grandchild1->ClearChangedToRoot();
    grandchild2->ClearChangedToRoot();
  }

  static void Update(scoped_refptr<ClipPaintPropertyNode> node,
                     scoped_refptr<const ClipPaintPropertyNode> new_parent,
                     const FloatRoundedRect& new_clip_rect) {
    node->Update(std::move(new_parent),
                 ClipPaintPropertyNode::State{nullptr, new_clip_rect});
  }

  void ExpectInitialState() {
    EXPECT_FALSE(root->Changed(*root));
    EXPECT_TRUE(node->Changed(*root));
    EXPECT_TRUE(child1->Changed(*node));
    EXPECT_TRUE(child2->Changed(*node));
    EXPECT_TRUE(grandchild1->Changed(*child1));
    EXPECT_TRUE(grandchild2->Changed(*child2));
  }

  void ExpectUnchangedState() {
    EXPECT_FALSE(root->Changed(*root));
    EXPECT_FALSE(node->Changed(*root));
    EXPECT_FALSE(child1->Changed(*root));
    EXPECT_FALSE(child2->Changed(*root));
    EXPECT_FALSE(grandchild1->Changed(*root));
    EXPECT_FALSE(grandchild2->Changed(*root));
  }

  scoped_refptr<ClipPaintPropertyNode> root;
  scoped_refptr<ClipPaintPropertyNode> node;
  scoped_refptr<ClipPaintPropertyNode> child1;
  scoped_refptr<ClipPaintPropertyNode> child2;
  scoped_refptr<ClipPaintPropertyNode> grandchild1;
  scoped_refptr<ClipPaintPropertyNode> grandchild2;
};

TEST_F(PaintPropertyNodeTest, LowestCommonAncestor) {
  EXPECT_EQ(node, &LowestCommonAncestor(*node, *node));
  EXPECT_EQ(root, &LowestCommonAncestor(*root, *root));

  EXPECT_EQ(node, &LowestCommonAncestor(*grandchild1, *grandchild2));
  EXPECT_EQ(node, &LowestCommonAncestor(*grandchild1, *child2));
  EXPECT_EQ(root, &LowestCommonAncestor(*grandchild1, *root));
  EXPECT_EQ(child1, &LowestCommonAncestor(*grandchild1, *child1));

  EXPECT_EQ(node, &LowestCommonAncestor(*grandchild2, *grandchild1));
  EXPECT_EQ(node, &LowestCommonAncestor(*grandchild2, *child1));
  EXPECT_EQ(root, &LowestCommonAncestor(*grandchild2, *root));
  EXPECT_EQ(child2, &LowestCommonAncestor(*grandchild2, *child2));

  EXPECT_EQ(node, &LowestCommonAncestor(*child1, *child2));
  EXPECT_EQ(node, &LowestCommonAncestor(*child2, *child1));
}

TEST_F(PaintPropertyNodeTest, InitialStateAndReset) {
  ExpectInitialState();
  ResetAllChanged();
  ExpectUnchangedState();
}

TEST_F(PaintPropertyNodeTest, ChangeNode) {
  ResetAllChanged();
  Update(node, root, FloatRoundedRect(1, 2, 3, 4));
  EXPECT_TRUE(node->Changed(*root));
  EXPECT_FALSE(node->Changed(*node));
  EXPECT_TRUE(child1->Changed(*root));
  EXPECT_FALSE(child1->Changed(*node));
  EXPECT_TRUE(grandchild1->Changed(*root));
  EXPECT_FALSE(grandchild1->Changed(*node));

  EXPECT_FALSE(grandchild1->Changed(*child2));
  EXPECT_FALSE(grandchild1->Changed(*grandchild2));

  ResetAllChanged();
  ExpectUnchangedState();
}

TEST_F(PaintPropertyNodeTest, ChangeOneChild) {
  ResetAllChanged();
  Update(child1, node, FloatRoundedRect(1, 2, 3, 4));
  EXPECT_FALSE(node->Changed(*root));
  EXPECT_FALSE(node->Changed(*node));
  EXPECT_TRUE(child1->Changed(*root));
  EXPECT_TRUE(child1->Changed(*node));
  EXPECT_TRUE(grandchild1->Changed(*node));
  EXPECT_FALSE(grandchild1->Changed(*child1));
  EXPECT_FALSE(child2->Changed(*node));
  EXPECT_FALSE(grandchild2->Changed(*node));

  EXPECT_TRUE(child2->Changed(*child1));
  EXPECT_TRUE(child1->Changed(*child2));
  EXPECT_TRUE(child2->Changed(*grandchild1));
  EXPECT_TRUE(child1->Changed(*grandchild2));
  EXPECT_TRUE(grandchild1->Changed(*child2));
  EXPECT_TRUE(grandchild1->Changed(*grandchild2));
  EXPECT_TRUE(grandchild2->Changed(*child1));
  EXPECT_TRUE(grandchild2->Changed(*grandchild1));

  ResetAllChanged();
  ExpectUnchangedState();
}

TEST_F(PaintPropertyNodeTest, Reparent) {
  ResetAllChanged();
  Update(child1, child2, FloatRoundedRect(1, 2, 3, 4));
  EXPECT_FALSE(node->Changed(*root));
  EXPECT_TRUE(child1->Changed(*node));
  EXPECT_TRUE(child1->Changed(*child2));
  EXPECT_FALSE(child2->Changed(*node));
  EXPECT_TRUE(grandchild1->Changed(*node));
  EXPECT_FALSE(grandchild1->Changed(*child1));
  EXPECT_TRUE(grandchild1->Changed(*child2));

  ResetAllChanged();
  ExpectUnchangedState();
}

}  // namespace blink

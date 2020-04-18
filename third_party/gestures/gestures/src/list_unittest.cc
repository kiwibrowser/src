// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <set>

#include <gtest/gtest.h>

#include "gestures/include/list.h"
#include "gestures/include/set.h"

namespace gestures {

class ListTest : public ::testing::Test {};

namespace {
struct Node {
  explicit Node() : val_(-1) {}
  explicit Node(int val) : val_(val) {}
  int val_;
  Node* next_;
  Node* prev_;
};

void VerifyList(List<Node>& list) {
  Node* first = list.Head()->prev_;  // sentinel
  Node* second = list.Head();  // next elt (sentinel or first element)
  std::set<Node*> seen_nodes;
  if (!list.Empty())
    EXPECT_NE(first, second);
  else
    EXPECT_EQ(first, second);
  seen_nodes.insert(second);
  for (size_t i = 0; i <= list.size(); ++i) {
    EXPECT_EQ(first->next_, second);
    EXPECT_EQ(second->prev_, first);
    if (i < list.size()) {
      first = second;
      second = second->next_;
      EXPECT_FALSE(SetContainsValue(seen_nodes, second));
      seen_nodes.insert(second);
    }
  }
  EXPECT_EQ(seen_nodes.size(), list.size() + 1);
  // second should now be sentinal tail
  Node* sen_tail = list.Tail()->next_;
  EXPECT_EQ(second, sen_tail);
}
}

TEST(ListTest, SimpleTest) {
  List<Node> list;
  VerifyList(list);
  EXPECT_TRUE(list.Empty());
  EXPECT_EQ(0, list.size());
  list.PushFront(new Node(4));
  VerifyList(list);
  EXPECT_FALSE(list.Empty());
  EXPECT_EQ(1, list.size());
  list.PushFront(new Node(5));
  VerifyList(list);
  EXPECT_EQ(2, list.size());
  EXPECT_EQ(4, list.Tail()->val_);
  EXPECT_EQ(5, list.Head()->val_);
  list.PushBack(new Node(3));
  VerifyList(list);
  EXPECT_EQ(3, list.size());

  Node* node = list.PopFront();
  VerifyList(list);
  ASSERT_TRUE(node);
  EXPECT_EQ(5, node->val_);
  delete node;

  node = list.PopBack();
  VerifyList(list);
  ASSERT_TRUE(node);
  EXPECT_EQ(3, node->val_);
  delete node;

  node = list.PopBack();
  VerifyList(list);
  ASSERT_TRUE(node);
  EXPECT_EQ(4, node->val_);
  delete node;
}

}  // namespace gestures

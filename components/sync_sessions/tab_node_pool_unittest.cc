// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_sessions/tab_node_pool.h"

#include <vector>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sync_sessions {

class SyncTabNodePoolTest : public testing::Test {
 protected:
  SyncTabNodePoolTest() {}

  int GetMaxUsedTabNodeId() const { return pool_.max_used_tab_node_id_; }

  void AddFreeTabNodes(const std::vector<int>& node_ids) {
    for (int node_id : node_ids) {
      pool_.free_nodes_pool_.insert(node_id);
    }
  }

  TabNodePool pool_;
};

namespace {

using testing::UnorderedElementsAre;

const int kTabNodeId1 = 10;
const int kTabNodeId2 = 5;
const int kTabNodeId3 = 1000;
const SessionID kTabId1 = SessionID::FromSerializedValue(1);
const SessionID kTabId2 = SessionID::FromSerializedValue(2);
const SessionID kTabId3 = SessionID::FromSerializedValue(3);

TEST_F(SyncTabNodePoolTest, TabNodeIdIncreases) {
  std::set<int> deleted_node_ids;

  // max_used_tab_node_ always increases.
  pool_.ReassociateTabNode(kTabNodeId1, kTabId1);
  EXPECT_EQ(kTabNodeId1, GetMaxUsedTabNodeId());
  pool_.ReassociateTabNode(kTabNodeId2, kTabId2);
  EXPECT_EQ(kTabNodeId1, GetMaxUsedTabNodeId());
  pool_.ReassociateTabNode(kTabNodeId3, kTabId3);
  EXPECT_EQ(kTabNodeId3, GetMaxUsedTabNodeId());
  // Freeing a tab node does not change max_used_tab_node_id_.
  pool_.FreeTab(kTabId3);
  pool_.CleanupTabNodes(&deleted_node_ids);
  EXPECT_TRUE(deleted_node_ids.empty());
  pool_.FreeTab(kTabId2);
  pool_.CleanupTabNodes(&deleted_node_ids);
  EXPECT_TRUE(deleted_node_ids.empty());
  pool_.FreeTab(kTabId1);
  pool_.CleanupTabNodes(&deleted_node_ids);
  EXPECT_TRUE(deleted_node_ids.empty());
  for (int i = 0; i < 3; ++i) {
    const SessionID tab_id = SessionID::FromSerializedValue(i + 1);
    ASSERT_EQ(TabNodePool::kInvalidTabNodeID,
              pool_.GetTabNodeIdFromTabId(tab_id));
    EXPECT_NE(TabNodePool::kInvalidTabNodeID,
              pool_.AssociateWithFreeTabNode(tab_id));
    EXPECT_EQ(kTabNodeId3, GetMaxUsedTabNodeId());
  }
  pool_.CleanupTabNodes(&deleted_node_ids);
  EXPECT_TRUE(deleted_node_ids.empty());
  EXPECT_EQ(kTabNodeId3, GetMaxUsedTabNodeId());
  EXPECT_TRUE(pool_.Empty());
}

TEST_F(SyncTabNodePoolTest, Reassociation) {
  // Reassociate tab node 1 with tab id 1.
  pool_.ReassociateTabNode(kTabNodeId1, kTabId1);
  EXPECT_EQ(1U, pool_.Capacity());
  EXPECT_TRUE(pool_.Empty());
  EXPECT_FALSE(pool_.Full());
  EXPECT_EQ(kTabId1, pool_.GetTabIdFromTabNodeId(kTabNodeId1));
  EXPECT_FALSE(pool_.GetTabIdFromTabNodeId(kTabNodeId2).is_valid());

  // Introduce a new tab node associated with the same tab. The old tab node
  // should get added to the free pool
  pool_.ReassociateTabNode(kTabNodeId2, kTabId1);
  EXPECT_EQ(2U, pool_.Capacity());
  EXPECT_FALSE(pool_.Empty());
  EXPECT_FALSE(pool_.Full());
  EXPECT_FALSE(pool_.GetTabIdFromTabNodeId(kTabNodeId1).is_valid());
  EXPECT_EQ(kTabId1, pool_.GetTabIdFromTabNodeId(kTabNodeId2));

  // Reassociating the same tab node/tab should have no effect.
  pool_.ReassociateTabNode(kTabNodeId2, kTabId1);
  EXPECT_EQ(2U, pool_.Capacity());
  EXPECT_FALSE(pool_.Empty());
  EXPECT_FALSE(pool_.Full());
  EXPECT_FALSE(pool_.GetTabIdFromTabNodeId(kTabNodeId1).is_valid());
  EXPECT_EQ(kTabId1, pool_.GetTabIdFromTabNodeId(kTabNodeId2));

  // Reassociating the new tab node with a new tab should just update the
  // association tables.
  pool_.ReassociateTabNode(kTabNodeId2, kTabId2);
  EXPECT_EQ(2U, pool_.Capacity());
  EXPECT_FALSE(pool_.Empty());
  EXPECT_FALSE(pool_.Full());
  EXPECT_FALSE(pool_.GetTabIdFromTabNodeId(kTabNodeId1).is_valid());
  EXPECT_EQ(kTabId2, pool_.GetTabIdFromTabNodeId(kTabNodeId2));

  // Reassociating the first tab node should make the pool empty.
  pool_.ReassociateTabNode(kTabNodeId1, kTabId1);
  EXPECT_EQ(2U, pool_.Capacity());
  EXPECT_TRUE(pool_.Empty());
  EXPECT_FALSE(pool_.Full());
  EXPECT_EQ(kTabId1, pool_.GetTabIdFromTabNodeId(kTabNodeId1));
  EXPECT_EQ(kTabId2, pool_.GetTabIdFromTabNodeId(kTabNodeId2));
}

TEST_F(SyncTabNodePoolTest, ReassociateThenFree) {
  std::set<int> deleted_node_ids;

  // Verify old tab nodes are reassociated correctly.
  pool_.ReassociateTabNode(kTabNodeId1, kTabId1);
  pool_.ReassociateTabNode(kTabNodeId2, kTabId2);
  pool_.ReassociateTabNode(kTabNodeId3, kTabId3);
  EXPECT_EQ(3u, pool_.Capacity());
  EXPECT_TRUE(pool_.Empty());
  // Free tabs 2 and 3.
  pool_.FreeTab(kTabId2);
  pool_.FreeTab(kTabId3);
  pool_.CleanupTabNodes(&deleted_node_ids);
  EXPECT_TRUE(deleted_node_ids.empty());
  // Free node pool should have 2 and 3.
  EXPECT_FALSE(pool_.Empty());
  EXPECT_EQ(3u, pool_.Capacity());

  // Free all nodes
  pool_.FreeTab(kTabId1);
  pool_.CleanupTabNodes(&deleted_node_ids);
  EXPECT_TRUE(deleted_node_ids.empty());
  EXPECT_TRUE(pool_.Full());

  // Reassociate tab nodes.
  std::vector<int> sync_ids;
  for (int i = 1; i <= 3; ++i) {
    const SessionID tab_id = SessionID::FromSerializedValue(i);
    EXPECT_EQ(TabNodePool::kInvalidTabNodeID,
              pool_.GetTabNodeIdFromTabId(tab_id));
    sync_ids.push_back(pool_.AssociateWithFreeTabNode(tab_id));
  }

  EXPECT_TRUE(pool_.Empty());
  EXPECT_THAT(sync_ids,
              UnorderedElementsAre(kTabNodeId1, kTabNodeId2, kTabNodeId3));
}

TEST_F(SyncTabNodePoolTest, Init) {
  EXPECT_TRUE(pool_.Empty());
  EXPECT_TRUE(pool_.Full());
}

TEST_F(SyncTabNodePoolTest, AddGet) {
  AddFreeTabNodes({5, 10});

  EXPECT_EQ(2U, pool_.Capacity());
  ASSERT_EQ(TabNodePool::kInvalidTabNodeID,
            pool_.GetTabNodeIdFromTabId(kTabId1));
  EXPECT_EQ(5, pool_.AssociateWithFreeTabNode(kTabId1));
  EXPECT_FALSE(pool_.Empty());
  EXPECT_FALSE(pool_.Full());
  EXPECT_EQ(2U, pool_.Capacity());
  ASSERT_EQ(TabNodePool::kInvalidTabNodeID,
            pool_.GetTabNodeIdFromTabId(kTabId2));
  // 5 is now used, should return 10.
  EXPECT_EQ(10, pool_.AssociateWithFreeTabNode(kTabId2));
}

TEST_F(SyncTabNodePoolTest, AssociateWithFreeTabNode) {
  ASSERT_EQ(TabNodePool::kInvalidTabNodeID,
            pool_.GetTabNodeIdFromTabId(kTabId1));
  ASSERT_EQ(TabNodePool::kInvalidTabNodeID,
            pool_.GetTabNodeIdFromTabId(kTabId2));
  EXPECT_EQ(0, pool_.AssociateWithFreeTabNode(kTabId1));
  EXPECT_EQ(0, pool_.GetTabNodeIdFromTabId(kTabId1));
  ASSERT_EQ(TabNodePool::kInvalidTabNodeID,
            pool_.GetTabNodeIdFromTabId(kTabId2));
  EXPECT_EQ(1, pool_.AssociateWithFreeTabNode(kTabId2));
  EXPECT_EQ(1, pool_.GetTabNodeIdFromTabId(kTabId2));
  pool_.FreeTab(kTabId1);
  EXPECT_EQ(0, pool_.AssociateWithFreeTabNode(kTabId3));
}

TEST_F(SyncTabNodePoolTest, TabPoolFreeNodeLimits) {
  std::set<int> deleted_node_ids;

  // Allocate TabNodePool::kFreeNodesHighWatermark + 1 nodes and verify that
  // freeing the last node reduces the free node pool size to
  // kFreeNodesLowWatermark.
  std::vector<int> used_sync_ids;
  for (size_t i = 1; i <= TabNodePool::kFreeNodesHighWatermark + 1; ++i) {
    used_sync_ids.push_back(
        pool_.AssociateWithFreeTabNode(SessionID::FromSerializedValue(i)));
  }

  // Free all except one node.
  used_sync_ids.pop_back();

  for (size_t i = 1; i <= used_sync_ids.size(); ++i) {
    pool_.FreeTab(SessionID::FromSerializedValue(i));
    pool_.CleanupTabNodes(&deleted_node_ids);
    EXPECT_TRUE(deleted_node_ids.empty());
  }

  // Except one node all nodes should be in FreeNode pool.
  EXPECT_FALSE(pool_.Full());
  EXPECT_FALSE(pool_.Empty());
  // Total capacity = 1 Associated Node + kFreeNodesHighWatermark free node.
  EXPECT_EQ(TabNodePool::kFreeNodesHighWatermark + 1, pool_.Capacity());

  // Freeing the last sync node should drop the free nodes to
  // kFreeNodesLowWatermark.
  pool_.FreeTab(
      SessionID::FromSerializedValue(TabNodePool::kFreeNodesHighWatermark + 1));
  pool_.CleanupTabNodes(&deleted_node_ids);
  EXPECT_EQ(TabNodePool::kFreeNodesHighWatermark + 1 -
                TabNodePool::kFreeNodesLowWatermark,
            deleted_node_ids.size());
  EXPECT_FALSE(pool_.Empty());
  EXPECT_TRUE(pool_.Full());
  EXPECT_EQ(TabNodePool::kFreeNodesLowWatermark, pool_.Capacity());
}

}  // namespace

}  // namespace sync_sessions

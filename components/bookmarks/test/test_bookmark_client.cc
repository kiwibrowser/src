// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/bookmarks/test/test_bookmark_client.h"

#include <stddef.h>

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_storage.h"

namespace bookmarks {

TestBookmarkClient::TestBookmarkClient() {}

TestBookmarkClient::~TestBookmarkClient() {}

// static
std::unique_ptr<BookmarkModel> TestBookmarkClient::CreateModel() {
  return CreateModelWithClient(base::WrapUnique(new TestBookmarkClient));
}

// static
std::unique_ptr<BookmarkModel> TestBookmarkClient::CreateModelWithClient(
    std::unique_ptr<BookmarkClient> client) {
  BookmarkClient* client_ptr = client.get();
  std::unique_ptr<BookmarkModel> bookmark_model(
      new BookmarkModel(std::move(client)));
  std::unique_ptr<BookmarkLoadDetails> details =
      std::make_unique<BookmarkLoadDetails>(client_ptr);
  details->LoadExtraNodes();
  details->CreateUrlIndex();
  bookmark_model->DoneLoading(std::move(details));
  return bookmark_model;
}

void TestBookmarkClient::SetExtraNodesToLoad(
    BookmarkPermanentNodeList extra_nodes) {
  extra_nodes_ = std::move(extra_nodes);
  // Keep a copy of the nodes in |unowned_extra_nodes_| for the accessor
  // functions.
  for (const auto& node : extra_nodes_)
    unowned_extra_nodes_.push_back(node.get());
}

bool TestBookmarkClient::IsExtraNodeRoot(const BookmarkNode* node) {
  return std::find(unowned_extra_nodes_.begin(), unowned_extra_nodes_.end(),
                   node) != unowned_extra_nodes_.end();
}

bool TestBookmarkClient::IsAnExtraNode(const BookmarkNode* node) {
  if (!node)
    return false;
  for (const auto* extra_node : unowned_extra_nodes_) {
    if (node->HasAncestor(extra_node))
      return true;
  }
  return false;
}

bool TestBookmarkClient::IsPermanentNodeVisible(
    const BookmarkPermanentNode* node) {
  DCHECK(node->type() == BookmarkNode::BOOKMARK_BAR ||
         node->type() == BookmarkNode::OTHER_NODE ||
         node->type() == BookmarkNode::MOBILE ||
         IsExtraNodeRoot(node));
  return node->type() != BookmarkNode::MOBILE && !IsExtraNodeRoot(node);
}

void TestBookmarkClient::RecordAction(const base::UserMetricsAction& action) {
}

LoadExtraCallback TestBookmarkClient::GetLoadExtraNodesCallback() {
  return base::Bind(&TestBookmarkClient::LoadExtraNodes,
                    base::Passed(&extra_nodes_));
}

bool TestBookmarkClient::CanSetPermanentNodeTitle(
    const BookmarkNode* permanent_node) {
  return IsExtraNodeRoot(permanent_node);
}

bool TestBookmarkClient::CanSyncNode(const BookmarkNode* node) {
  return !IsAnExtraNode(node);
}

bool TestBookmarkClient::CanBeEditedByUser(const BookmarkNode* node) {
  return !IsAnExtraNode(node);
}

// static
BookmarkPermanentNodeList TestBookmarkClient::LoadExtraNodes(
    BookmarkPermanentNodeList extra_nodes,
    int64_t* next_id) {
  return extra_nodes;
}

}  // namespace bookmarks

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/mock_browsing_data_channel_id_helper.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/stl_util.h"
#include "testing/gtest/include/gtest/gtest.h"

MockBrowsingDataChannelIDHelper::MockBrowsingDataChannelIDHelper()
    : BrowsingDataChannelIDHelper() {}

MockBrowsingDataChannelIDHelper::
~MockBrowsingDataChannelIDHelper() {}

void MockBrowsingDataChannelIDHelper::StartFetching(
    const FetchResultCallback& callback) {
  ASSERT_FALSE(callback.is_null());
  ASSERT_TRUE(callback_.is_null());
  callback_ = callback;
}

void MockBrowsingDataChannelIDHelper::DeleteChannelID(
    const std::string& server_id) {
  ASSERT_FALSE(callback_.is_null());
  ASSERT_TRUE(base::ContainsKey(channel_ids_, server_id));
  channel_ids_[server_id] = false;
}

void MockBrowsingDataChannelIDHelper::AddChannelIDSample(
    const std::string& server_id) {
  ASSERT_FALSE(base::ContainsKey(channel_ids_, server_id));
  std::unique_ptr<crypto::ECPrivateKey> key(crypto::ECPrivateKey::Create());
  channel_id_list_.push_back(
      net::ChannelIDStore::ChannelID(server_id, base::Time(), std::move(key)));
  channel_ids_[server_id] = true;
}

void MockBrowsingDataChannelIDHelper::Notify() {
  net::ChannelIDStore::ChannelIDList channel_id_list;
  for (const auto& channel_id : channel_id_list_) {
    if (channel_ids_.at(channel_id.server_identifier()))
      channel_id_list.push_back(channel_id);
  }
  callback_.Run(channel_id_list);
}

void MockBrowsingDataChannelIDHelper::Reset() {
  for (auto& pair : channel_ids_)
    pair.second = true;
}

bool MockBrowsingDataChannelIDHelper::AllDeleted() {
  for (const auto& pair : channel_ids_) {
    if (pair.second)
      return false;
  }
  return true;
}

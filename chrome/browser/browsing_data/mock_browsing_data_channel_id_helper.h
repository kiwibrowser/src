// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_CHANNEL_ID_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_CHANNEL_ID_HELPER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "chrome/browser/browsing_data/browsing_data_channel_id_helper.h"

// Mock for BrowsingDataChannelIDHelper.
class MockBrowsingDataChannelIDHelper
    : public BrowsingDataChannelIDHelper {
 public:
  MockBrowsingDataChannelIDHelper();

  // BrowsingDataChannelIDHelper methods.
  void StartFetching(const FetchResultCallback& callback) override;
  void DeleteChannelID(const std::string& server_id) override;

  // Adds a channel_id sample.
  void AddChannelIDSample(const std::string& server_id);

  // Notifies the callback.
  void Notify();

  // Marks all channel_ids as existing.
  void Reset();

  // Returns true if all channel_ids since the last Reset() invocation
  // were deleted.
  bool AllDeleted();

 private:
  ~MockBrowsingDataChannelIDHelper() override;

  FetchResultCallback callback_;

  net::ChannelIDStore::ChannelIDList channel_id_list_;

  // Stores which channel_ids exist.
  std::map<const std::string, bool> channel_ids_;

  DISALLOW_COPY_AND_ASSIGN(MockBrowsingDataChannelIDHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_CHANNEL_ID_HELPER_H_

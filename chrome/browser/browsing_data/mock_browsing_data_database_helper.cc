// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/mock_browsing_data_database_helper.h"

#include "base/callback.h"
#include "base/stl_util.h"
#include "testing/gtest/include/gtest/gtest.h"

MockBrowsingDataDatabaseHelper::MockBrowsingDataDatabaseHelper(
    Profile* profile)
    : BrowsingDataDatabaseHelper(profile) {
}

MockBrowsingDataDatabaseHelper::~MockBrowsingDataDatabaseHelper() {
}

void MockBrowsingDataDatabaseHelper::StartFetching(FetchCallback callback) {
  callback_ = std::move(callback);
}

void MockBrowsingDataDatabaseHelper::DeleteDatabase(
    const std::string& origin,
    const std::string& name) {
  std::string key = origin + ":" + name;
  ASSERT_TRUE(base::ContainsKey(databases_, key));
  last_deleted_origin_ = origin;
  last_deleted_db_ = name;
  databases_[key] = false;
}

void MockBrowsingDataDatabaseHelper::AddDatabaseSamples() {
  storage::DatabaseIdentifier id1 =
      storage::DatabaseIdentifier::Parse("http_gdbhost1_1");
  response_.push_back(BrowsingDataDatabaseHelper::DatabaseInfo(
      id1, "db1", "description 1", 1, base::Time()));
  databases_["http_gdbhost1_1:db1"] = true;
  storage::DatabaseIdentifier id2 =
      storage::DatabaseIdentifier::Parse("http_gdbhost2_2");
  response_.push_back(BrowsingDataDatabaseHelper::DatabaseInfo(
      id2, "db2", "description 2", 2, base::Time()));
  databases_["http_gdbhost2_2:db2"] = true;
}

void MockBrowsingDataDatabaseHelper::Notify() {
  std::move(callback_).Run(response_);
}

void MockBrowsingDataDatabaseHelper::Reset() {
  for (auto& pair : databases_)
    pair.second = true;
}

bool MockBrowsingDataDatabaseHelper::AllDeleted() {
  for (const auto& pair : databases_) {
    if (pair.second)
      return false;
  }
  return true;
}

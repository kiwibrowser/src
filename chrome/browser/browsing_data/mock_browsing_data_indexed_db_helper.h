// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_INDEXED_DB_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_INDEXED_DB_HELPER_H_

#include <list>
#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/browsing_data/browsing_data_indexed_db_helper.h"

class Profile;

// Mock for BrowsingDataIndexedDBHelper.
// Use AddIndexedDBSamples() or add directly to response_ list, then
// call Notify().
class MockBrowsingDataIndexedDBHelper
    : public BrowsingDataIndexedDBHelper {
 public:
  explicit MockBrowsingDataIndexedDBHelper(Profile* profile);

  // Adds some IndexedDBInfo samples.
  void AddIndexedDBSamples();

  // Notifies the callback.
  void Notify();

  // Marks all indexed db files as existing.
  void Reset();

  // Returns true if all indexed db files were deleted since the last
  // Reset() invokation.
  bool AllDeleted();

  // BrowsingDataIndexedDBHelper.
  void StartFetching(const FetchCallback& callback) override;
  void DeleteIndexedDB(const GURL& origin) override;

 private:
  ~MockBrowsingDataIndexedDBHelper() override;

  FetchCallback callback_;
  std::map<GURL, bool> origins_;
  std::list<content::IndexedDBInfo> response_;

  DISALLOW_COPY_AND_ASSIGN(MockBrowsingDataIndexedDBHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_INDEXED_DB_HELPER_H_

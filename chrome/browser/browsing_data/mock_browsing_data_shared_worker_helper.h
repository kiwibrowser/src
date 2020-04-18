// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_SHARED_WORKER_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_SHARED_WORKER_HELPER_H_

#include <list>
#include <map>

#include "chrome/browser/browsing_data/browsing_data_shared_worker_helper.h"

class Profile;

class MockBrowsingDataSharedWorkerHelper
    : public BrowsingDataSharedWorkerHelper {
 public:
  explicit MockBrowsingDataSharedWorkerHelper(Profile* profile);

  // Adds some shared worker samples.
  void AddSharedWorkerSamples();

  // Notifies the callback.
  void Notify();

  // Marks all shared worker files as existing.
  void Reset();

  // Returns true if all shared worker files were deleted since the last
  // Reset() invokation.
  bool AllDeleted();

  // BrowsingDataSharedWorkerHelper methods.
  void StartFetching(FetchCallback callback) override;
  void DeleteSharedWorker(const GURL& worker,
                          const std::string& name,
                          const url::Origin& constructor_origin) override;

 private:
  ~MockBrowsingDataSharedWorkerHelper() override;

  FetchCallback callback_;
  std::map<SharedWorkerInfo, bool> workers_;
  std::list<SharedWorkerInfo> response_;

  DISALLOW_COPY_AND_ASSIGN(MockBrowsingDataSharedWorkerHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_MOCK_BROWSING_DATA_SHARED_WORKER_HELPER_H_

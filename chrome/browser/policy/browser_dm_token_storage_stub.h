// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_STUB_H_
#define CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_STUB_H_

#include "chrome/browser/policy/browser_dm_token_storage.h"

#include <string>

#include "base/macros.h"

namespace policy {

// No-op implementation of BrowserDMTokenStorage. The global singleton instance
// can be retrieved by calling BrowserDMTokenStorage::Get().
class BrowserDMTokenStorageStub : public BrowserDMTokenStorage {
 public:
  // Get the global singleton instance by calling BrowserDMTokenStorage::Get().
  BrowserDMTokenStorageStub() = default;
  ~BrowserDMTokenStorageStub() override = default;

  // override BrowserDMTokenStorage
  std::string RetrieveClientId() override;
  std::string RetrieveEnrollmentToken() override;
  void StoreDMToken(const std::string& dm_token,
                    StoreCallback callback) override;
  std::string RetrieveDMToken() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserDMTokenStorageStub);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_STUB_H_

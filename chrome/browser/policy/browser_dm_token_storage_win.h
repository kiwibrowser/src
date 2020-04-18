// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_WIN_H_
#define CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_WIN_H_

#include "chrome/browser/policy/browser_dm_token_storage.h"

#include <string>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"

namespace policy {

// Implementation of BrowserDMTokenStorage for Windows. The global singleton
// instance can be retrieved by calling BrowserDMTokenStorage::Get().
class BrowserDMTokenStorageWin : public BrowserDMTokenStorage {
 public:
  // Get the global singleton instance by calling BrowserDMTokenStorage::Get().
  BrowserDMTokenStorageWin();
  ~BrowserDMTokenStorageWin() override;

  // override BrowserDMTokenStorage
  std::string RetrieveClientId() override;
  std::string RetrieveEnrollmentToken() override;
  void StoreDMToken(const std::string& dm_token,
                    StoreCallback callback) override;
  std::string RetrieveDMToken() override;

 private:
  // Initialize the DMTokenStorage, reads the |enrollment_token_| and
  // |dm_token_| from Registry synchronously.
  void InitIfNeeded();

  void OnDMTokenStored(bool success);

  scoped_refptr<base::SingleThreadTaskRunner> com_sta_task_runner_;
  StoreCallback store_callback_;

  bool is_initialized_;

  std::string client_id_;
  std::string enrollment_token_;
  std::string dm_token_;

  SEQUENCE_CHECKER(sequence_checker_);

  // This should always be the last member of the class.
  base::WeakPtrFactory<BrowserDMTokenStorageWin> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BrowserDMTokenStorageWin);
};

}  // namespace policy
#endif  // CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_WIN_H_

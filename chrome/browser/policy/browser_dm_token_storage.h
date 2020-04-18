// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_H_
#define CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"

namespace policy {

// Manages storing and retrieving tokens and client ID used to enroll browser
// instances for enterprise management. The tokens are read from disk or
// registry once and cached values are returned in subsequent calls.
//
// All calls to member functions must be sequenced. It is an error to attempt
// concurrent store operations.
class BrowserDMTokenStorage {
 public:
  using StoreCallback = base::OnceCallback<void(bool success)>;

  // Returns the global singleton object. Must be called from the UI thread.
  static BrowserDMTokenStorage* Get();
  // Returns a client ID unique to the machine.
  virtual std::string RetrieveClientId() = 0;
  // Returns the enrollment token, or an empty string if there is none.
  virtual std::string RetrieveEnrollmentToken() = 0;
  // Asynchronously stores |dm_token| in the registry and calls |callback| with
  // a boolean to indicate success or failure. It is an error to attempt
  // concurrent store operations.
  virtual void StoreDMToken(const std::string& dm_token,
                            StoreCallback callback) = 0;
  // Returns an already stored DM token from the registry or from the cache in
  // memory. An empty token is returned if no DM token exists on the system or
  // an error is encountered.
  virtual std::string RetrieveDMToken() = 0;

  // Set the mock BrowserDMTokenStorage for testing. The caller owns the
  // instance of the storage.
  static void SetForTesting(BrowserDMTokenStorage* storage) {
    storage_for_testing_ = storage;
  }

 protected:
  BrowserDMTokenStorage() = default;
  virtual ~BrowserDMTokenStorage() = default;

 private:
  static BrowserDMTokenStorage* storage_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(BrowserDMTokenStorage);
};

}  // namespace policy
#endif  // CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_H_

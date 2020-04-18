// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_DB_WHITELIST_CHECKER_CLIENT_H_
#define COMPONENTS_SAFE_BROWSING_DB_WHITELIST_CHECKER_CLIENT_H_

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "components/safe_browsing/db/database_manager.h"

namespace safe_browsing {

// This provides a simpler interface to
// SafeBrowsingDatabaseManager::CheckCsdWhitelistUrl() for callers that
// don't want to track their own clients.

class WhitelistCheckerClient : public SafeBrowsingDatabaseManager::Client {
 public:
  using BoolCallback = base::Callback<void(bool /* is_whitelisted */)>;

  // Static method to instantiate and start a check. The callback will
  // be invoked when it's done, times out, or if database_manager gets
  // shut down. Must be called on IO thread.
  static void StartCheckCsdWhitelist(
      scoped_refptr<SafeBrowsingDatabaseManager> database_manager,
      const GURL& url,
      BoolCallback callback_for_result);

  WhitelistCheckerClient(
      BoolCallback callback_for_result,
      scoped_refptr<SafeBrowsingDatabaseManager> database_manager);
  ~WhitelistCheckerClient() override;

  // SafeBrowsingDatabaseMananger::Client impl
  void OnCheckWhitelistUrlResult(bool is_whitelisted) override;

 protected:
  static const int kTimeoutMsec = 5000;
  base::OneShotTimer timer_;
  BoolCallback callback_for_result_;
  scoped_refptr<SafeBrowsingDatabaseManager> database_manager_;
  base::WeakPtrFactory<WhitelistCheckerClient> weak_factory_;

 private:
  WhitelistCheckerClient();

  // Called when the call to CheckCsdWhitelistUrl times out.
  void OnCheckWhitelistUrlTimeout();
};

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_DB_WHITELIST_CHECKER_CLIENT_H_

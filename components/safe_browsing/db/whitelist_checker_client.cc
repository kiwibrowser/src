// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/db/whitelist_checker_client.h"

#include <memory>

#include "base/bind.h"

namespace safe_browsing {

// Static
void WhitelistCheckerClient::StartCheckCsdWhitelist(
    scoped_refptr<SafeBrowsingDatabaseManager> database_manager,
    const GURL& url,
    base::Callback<void(bool)> callback_for_result) {
  // TODO(nparker): Maybe also call SafeBrowsingDatabaseManager::CanCheckUrl()
  if (!url.is_valid()) {
    callback_for_result.Run(true /* is_whitelisted */);
    return;
  }

  // Make a client for each request. The caller could have several in
  // flight at once.
  std::unique_ptr<WhitelistCheckerClient> client =
      std::make_unique<WhitelistCheckerClient>(callback_for_result,
                                               database_manager);
  AsyncMatch match = database_manager->CheckCsdWhitelistUrl(url, client.get());

  switch (match) {
    case AsyncMatch::MATCH:
      callback_for_result.Run(true /* is_whitelisted */);
      break;
    case AsyncMatch::NO_MATCH:
      callback_for_result.Run(false /* is_whitelisted */);
      break;
    case AsyncMatch::ASYNC:
      // Client is now self-owned. When it gets called back with the result,
      // it will delete itself.
      client.release();
      break;
  }
}

WhitelistCheckerClient::WhitelistCheckerClient(
    base::Callback<void(bool)> callback_for_result,
    scoped_refptr<SafeBrowsingDatabaseManager> database_manager)
    : callback_for_result_(callback_for_result),
      database_manager_(database_manager),
      weak_factory_(this) {
  // Set a timer to fail open, i.e. call it "whitelisted", if the full
  // check takes too long.
  auto timeout_callback =
      base::Bind(&WhitelistCheckerClient::OnCheckWhitelistUrlTimeout,
                 weak_factory_.GetWeakPtr());
  timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(kTimeoutMsec),
               timeout_callback);
}

WhitelistCheckerClient::~WhitelistCheckerClient() {}

// SafeBrowsingDatabaseMananger::Client impl
void WhitelistCheckerClient::OnCheckWhitelistUrlResult(bool is_whitelisted) {
  timer_.Stop();
  callback_for_result_.Run(is_whitelisted);
  // This method is invoked only if we're already self-owned.
  delete this;
}

void WhitelistCheckerClient::OnCheckWhitelistUrlTimeout() {
  database_manager_->CancelCheck(this);
  this->OnCheckWhitelistUrlResult(true /* is_whitelisted */);
}

}  // namespace safe_browsing

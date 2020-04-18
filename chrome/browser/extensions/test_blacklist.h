// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_TEST_BLACKLIST_H_
#define CHROME_BROWSER_EXTENSIONS_TEST_BLACKLIST_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "chrome/browser/extensions/blacklist.h"
#include "chrome/browser/extensions/blacklist_state_fetcher.h"

namespace extensions {

class FakeSafeBrowsingDatabaseManager;

// Replace BlacklistStateFetcher for testing of the Blacklist class.
class BlacklistStateFetcherMock : public BlacklistStateFetcher {
 public:
  BlacklistStateFetcherMock();

  ~BlacklistStateFetcherMock() override;

  void Request(const std::string& id, const RequestCallback& callback) override;

  void SetState(const std::string& id, BlacklistState state);

  void Clear();

  int request_count() const { return request_count_; }

 private:
  std::map<std::string, BlacklistState> states_;
  int request_count_;
};


// A wrapper for an extensions::Blacklist that provides functionality for
// testing. It sets up mocks for SafeBrowsing database and BlacklistFetcher,
// that are used by blacklist to retrieve respectively the set of blacklisted
// extensions and their blacklist states.
class TestBlacklist {
 public:
  // Use this if the SafeBrowsing and/or StateFetcher mocks should be created
  // before initializing the Blacklist.
  explicit TestBlacklist();

  explicit TestBlacklist(Blacklist* blacklist);

  ~TestBlacklist();

  void Attach(Blacklist* blacklist);

  // Only call this if Blacklist is destroyed before TestBlacklist, otherwise
  // it will be performed from the destructor.
  void Detach();

  Blacklist* blacklist() { return blacklist_; }

  // Set the extension state in SafeBrowsingDatabaseManager and
  // BlacklistFetcher.
  void SetBlacklistState(const std::string& extension_id,
                         BlacklistState state,
                         bool notify);

  BlacklistState GetBlacklistState(const std::string& extension_id);

  void Clear(bool notify);

  void DisableSafeBrowsing();

  void EnableSafeBrowsing();

  void NotifyUpdate();

  const BlacklistStateFetcherMock* fetcher() { return &state_fetcher_mock_; }

 private:
  Blacklist* blacklist_;

  // The BlacklistStateFetcher object is normally managed by Blacklist. Because
  // of this, we need to prevent this object from being deleted with Blacklist.
  // For this, Detach() should be called before blacklist_ is deleted.
  BlacklistStateFetcherMock state_fetcher_mock_;

  scoped_refptr<FakeSafeBrowsingDatabaseManager> blacklist_db_;

  Blacklist::ScopedDatabaseManagerForTest scoped_blacklist_db_;

  DISALLOW_COPY_AND_ASSIGN(TestBlacklist);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_TEST_BLACKLIST_H_

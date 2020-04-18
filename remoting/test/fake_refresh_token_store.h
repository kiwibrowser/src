// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_FAKE_REFRESH_TOKEN_STORE_H_
#define REMOTING_TEST_FAKE_REFRESH_TOKEN_STORE_H_

#include "base/macros.h"
#include "remoting/test/refresh_token_store.h"

namespace remoting {
namespace test {

// Stubs out the file API and returns fake data so we can remove
// file system dependencies when testing the TestDriverEnvironment.
class FakeRefreshTokenStore : public RefreshTokenStore {
 public:
  FakeRefreshTokenStore();
  ~FakeRefreshTokenStore() override;

  // RefreshTokenStore interface.
  std::string FetchRefreshToken() override;
  bool StoreRefreshToken(const std::string& refresh_token) override;

  bool refresh_token_write_attempted() const {
    return refresh_token_write_attempted_;
  }

  const std::string& stored_refresh_token_value() const {
    return stored_refresh_token_value_;
  }

  void set_refresh_token_value(const std::string& new_token_value) {
    refresh_token_value_ = new_token_value;
  }

  void set_refresh_token_write_succeeded(bool write_succeeded) {
    refresh_token_write_succeeded_ = write_succeeded;
  }

 private:
  // Control members used to return specific data to the caller.
  std::string refresh_token_value_;
  bool refresh_token_write_succeeded_;

  // Verification members to observe the value of the data being written.
  bool refresh_token_write_attempted_;
  std::string stored_refresh_token_value_;

  DISALLOW_COPY_AND_ASSIGN(FakeRefreshTokenStore);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_FAKE_REFRESH_TOKEN_STORE_H_

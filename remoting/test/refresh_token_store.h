// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_REFRESH_TOKEN_STORE_H_
#define REMOTING_TEST_REFRESH_TOKEN_STORE_H_

#include <memory>
#include <string>


namespace base {
class FilePath;
}

namespace remoting {
namespace test {

// Used to store and retrieve refresh tokens.  This interface is provided to
// allow for stubbing out the storage mechanism for testing.
class RefreshTokenStore {
 public:
  RefreshTokenStore() {}
  virtual ~RefreshTokenStore() {}

  virtual std::string FetchRefreshToken() = 0;
  virtual bool StoreRefreshToken(const std::string& refresh_token) = 0;

  // Returns a RefreshTokenStore which reads/writes to a user specific token
  // file on the local disk.
  static std::unique_ptr<RefreshTokenStore> OnDisk(
      const std::string& user_name,
      const base::FilePath& refresh_token_file_path);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_REFRESH_TOKEN_STORE_H_

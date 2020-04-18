// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_FAKE_REMOTE_HOST_INFO_FETCHER_H_
#define REMOTING_TEST_FAKE_REMOTE_HOST_INFO_FETCHER_H_

#include <string>

#include "base/macros.h"
#include "remoting/test/remote_host_info_fetcher.h"

namespace remoting {
namespace test {

// Used for testing classes which rely on the RemoteHostInfoFetcher and want to
// simulate success and failure scenarios without using the actual class and
// network connection.
class FakeRemoteHostInfoFetcher : public RemoteHostInfoFetcher {
 public:
  FakeRemoteHostInfoFetcher();
  ~FakeRemoteHostInfoFetcher() override;

  // RemoteHostInfoFetcher interface.
  bool RetrieveRemoteHostInfo(const std::string& application_id,
                              const std::string& access_token,
                              ServiceEnvironment service_environment,
                              const RemoteHostInfoCallback& callback) override;

  void set_fail_retrieve_remote_host_info(bool fail) {
    fail_retrieve_remote_host_info_ = fail;
  }

 private:
  // True if RetrieveRemoteHostInfo() should fail.
  bool fail_retrieve_remote_host_info_;

  DISALLOW_COPY_AND_ASSIGN(FakeRemoteHostInfoFetcher);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_FAKE_REMOTE_HOST_INFO_FETCHER_H_

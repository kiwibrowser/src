// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_FAKE_HOST_LIST_FETCHER_H_
#define REMOTING_TEST_FAKE_HOST_LIST_FETCHER_H_

#include "base/macros.h"
#include "remoting/test/host_list_fetcher.h"

namespace remoting {
namespace test {

// Used to fake retrieving the host list without making any actual calls to the
// directory service for information.
class FakeHostListFetcher : public HostListFetcher {
 public:
  FakeHostListFetcher();
  ~FakeHostListFetcher() override;

  // HostListFetcher interface.
  void RetrieveHostlist(const std::string& access_token,
                        const std::string& target_url,
                        const HostlistCallback& callback) override;

  void set_retrieved_host_list(const std::vector<HostInfo>& host_list) {
    host_list_ = host_list;
  }

 private:
  std::vector<HostInfo> host_list_;

  DISALLOW_COPY_AND_ASSIGN(FakeHostListFetcher);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_FAKE_HOST_LIST_FETCHER_H_

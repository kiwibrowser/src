// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/fake_host_list_fetcher.h"

namespace remoting {
namespace test {

FakeHostListFetcher::FakeHostListFetcher() = default;

FakeHostListFetcher::~FakeHostListFetcher() = default;

void FakeHostListFetcher::RetrieveHostlist(const std::string& access_token,
                                           const std::string& target_url,
                                           const HostlistCallback& callback) {
  callback.Run(host_list_);
}

}  // namespace test
}  // namespace remoting

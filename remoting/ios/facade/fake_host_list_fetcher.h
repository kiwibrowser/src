// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_FACADE_FAKE_HOST_LIST_FETCHER_H_
#define REMOTING_IOS_FACADE_FAKE_HOST_LIST_FETCHER_H_

#include "base/macros.h"
#include "remoting/ios/facade/host_list_fetcher.h"

namespace remoting {

// Used to fake retrieving the host list without making any actual calls to the
// directory service for information.
class FakeHostListFetcher : public HostListFetcher {
 public:
  FakeHostListFetcher();
  ~FakeHostListFetcher() override;

  // HostListFetcher interface.
  void RetrieveHostlist(const std::string& access_token,
                        HostlistCallback callback) override;

  void ResolveCallback(int response_code,
                       const std::vector<HostInfo>& host_list);

  void CancelFetch() override;
  void ExpectCancelFetch();

 private:
  HostlistCallback callback_;
  bool cancel_fetch_expected_ = false;

  DISALLOW_COPY_AND_ASSIGN(FakeHostListFetcher);
};

}  // namespace remoting

#endif  // REMOTING_IOS_FACADE_FAKE_HOST_LIST_FETCHER_H_

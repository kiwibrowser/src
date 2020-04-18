// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/ios/facade/fake_host_list_fetcher.h"

namespace remoting {

FakeHostListFetcher::FakeHostListFetcher() : HostListFetcher(nullptr){};

FakeHostListFetcher::~FakeHostListFetcher(){};

void FakeHostListFetcher::RetrieveHostlist(const std::string& access_token,
                                           HostlistCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
}

void FakeHostListFetcher::ResolveCallback(
    int response_code,
    const std::vector<HostInfo>& host_list) {
  DCHECK(callback_);
  std::move(callback_).Run(response_code, host_list);
}

void FakeHostListFetcher::CancelFetch() {
  DCHECK(cancel_fetch_expected_);
  cancel_fetch_expected_ = false;
  if (callback_) {
    std::move(callback_).Run(RESPONSE_CODE_CANCELLED, {});
  }
}

void FakeHostListFetcher::ExpectCancelFetch() {
  DCHECK(!cancel_fetch_expected_);
  cancel_fetch_expected_ = true;
}

}  // namespace remoting

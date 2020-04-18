// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/fake_remote_host_info_fetcher.h"

namespace remoting {
namespace test {

FakeRemoteHostInfoFetcher::FakeRemoteHostInfoFetcher()
    : fail_retrieve_remote_host_info_(false) {
}

FakeRemoteHostInfoFetcher::~FakeRemoteHostInfoFetcher() = default;

bool FakeRemoteHostInfoFetcher::RetrieveRemoteHostInfo(
    const std::string& application_id,
    const std::string& access_token,
    ServiceEnvironment service_environment,
    const RemoteHostInfoCallback& callback) {
  RemoteHostInfo remote_host_info;

  if (fail_retrieve_remote_host_info_) {
    remote_host_info.remote_host_status = kRemoteHostStatusPending;
  } else {
    remote_host_info.remote_host_status = kRemoteHostStatusReady;
    remote_host_info.application_id = application_id;
  }

  callback.Run(remote_host_info);

  return !fail_retrieve_remote_host_info_;
}

}  // namespace test
}  // namespace remoting

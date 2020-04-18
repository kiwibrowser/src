// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/remote_host_info.h"

#include "base/logging.h"

namespace {
const char kAppRemotingCapabilities[] =
    "rateLimitResizeRequests desktopShape sendInitialResolution googleDrive";
const char kFakeHostPublicKey[] = "FAKE_HOST_PUBLIC_KEY";
}

namespace remoting {
namespace test {

RemoteHostInfo::RemoteHostInfo()
    : remote_host_status(kRemoteHostStatusUnknown) {}

RemoteHostInfo::~RemoteHostInfo() = default;

bool RemoteHostInfo::IsReadyForConnection() const {
  return remote_host_status == kRemoteHostStatusReady;
}

void RemoteHostInfo::SetRemoteHostStatusFromString(
    const std::string& status_string) {
  if (status_string == "done") {
    remote_host_status = kRemoteHostStatusReady;
  } else if (status_string == "pending") {
    remote_host_status = kRemoteHostStatusPending;
  } else {
    LOG(WARNING) << "Unknown status passed in: " << status_string;
    remote_host_status = kRemoteHostStatusUnknown;
  }
}

ConnectionSetupInfo RemoteHostInfo::GenerateConnectionSetupInfo(
    const std::string& access_token,
    const std::string& user_name) const {
  ConnectionSetupInfo connection_setup_info;
  connection_setup_info.access_token = access_token;
  connection_setup_info.authorization_code = authorization_code;
  connection_setup_info.capabilities = kAppRemotingCapabilities;
  connection_setup_info.host_id = host_id;
  connection_setup_info.host_jid = host_jid;
  connection_setup_info.public_key = kFakeHostPublicKey;
  connection_setup_info.shared_secret = shared_secret;
  connection_setup_info.user_name = user_name;

  return connection_setup_info;
}

}  // namespace test
}  // namespace remoting

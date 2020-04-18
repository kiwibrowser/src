// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_REMOTE_HOST_INFO_H_
#define REMOTING_TEST_REMOTE_HOST_INFO_H_

#include <string>

#include "remoting/test/connection_setup_info.h"

namespace remoting {
namespace test {

enum RemoteHostStatus {
  kRemoteHostStatusReady,
  kRemoteHostStatusPending,
  kRemoteHostStatusUnknown
};

// Holds the information needed to establish a connection with a remote host.
struct RemoteHostInfo {
  RemoteHostInfo();
  ~RemoteHostInfo();

  // Returns true if the remote host is ready to accept connections.
  bool IsReadyForConnection() const;

  // Sets the |remote_host_status| based on the caller supplied string.
  void SetRemoteHostStatusFromString(const std::string& status_string);

  // Generates connection information to establish a chromoting connection.
  ConnectionSetupInfo GenerateConnectionSetupInfo(
      const std::string& access_token,
      const std::string& user_name) const;

  // Data used to establish a connection with a remote host.
  RemoteHostStatus remote_host_status;
  std::string application_id;
  std::string host_id;
  std::string host_jid;
  std::string authorization_code;
  std::string shared_secret;
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_REMOTE_HOST_INFO_H_

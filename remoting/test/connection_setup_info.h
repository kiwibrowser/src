// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_CONNECTION_SETUP_INFO_H_
#define REMOTING_TEST_CONNECTION_SETUP_INFO_H_

#include <string>

namespace remoting {
namespace test {

// Holds the information needed to establish a connection with a remote host.
struct ConnectionSetupInfo {
  ConnectionSetupInfo();
  ConnectionSetupInfo(const ConnectionSetupInfo& other);
  ~ConnectionSetupInfo();

  // User provided information.
  std::string access_token;
  std::string user_name;
  std::string pin;

  // Chromoting host information.
  std::string host_name;
  std::string offline_reason;
  std::string public_key;

  // App Remoting information.
  std::string authorization_code;
  std::string shared_secret;

  // Chromoting host information.
  std::string capabilities;
  std::string host_id;
  std::string host_jid;
  std::string pairing_id;
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_CONNECTION_SETUP_INFO_H_

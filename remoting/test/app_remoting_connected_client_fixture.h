// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_APP_REMOTING_CONNECTED_CLIENT_FIXTURE_H_
#define REMOTING_TEST_APP_REMOTING_CONNECTED_CLIENT_FIXTURE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "remoting/test/remote_connection_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class RunLoop;
class Timer;
}

namespace remoting {
namespace test {

struct RemoteApplicationDetails;
class AppRemotingConnectionHelper;

// Allows for custom handling of ExtensionMessage messages.
typedef base::Callback<void(const protocol::ExtensionMessage& message)>
    HostMessageReceivedCallback;

// Creates a connection to a remote host which is available for tests to use.
// All callbacks must occur on the same thread the object was created on.
class AppRemotingConnectedClientFixture
    : public testing::TestWithParam<const char*>,
      public RemoteConnectionObserver {
 public:
  AppRemotingConnectedClientFixture();
  ~AppRemotingConnectedClientFixture() override;

 protected:
  // Sends the request to the host and waits for a reply up to |max_wait_time|.
  // Returns true if we received a response within the maximum time limit.
  bool VerifyResponseForSimpleHostMessage(
      const std::string& message_request_title,
      const std::string& message_response_title,
      const std::string& message_payload,
      const base::TimeDelta& max_wait_time);

 private:
  // testing::Test interface.
  void SetUp() override;
  void TearDown() override;

  // RemoteConnectionObserver interface.
  void HostMessageReceived(const protocol::ExtensionMessage& message) override;

  // Handles messages from the host.
  void HandleOnHostMessage(const remoting::protocol::ExtensionMessage& message);

  // Contains the details for the application being tested.
  const RemoteApplicationDetails& application_details_;

  // Used to run the thread's message loop.
  std::unique_ptr<base::RunLoop> run_loop_;

  // Used for setting timeouts and delays.
  std::unique_ptr<base::Timer> timer_;

  // Used to ensure RemoteConnectionObserver methods are called on the same
  // thread.
  base::ThreadChecker thread_checker_;

  // Creates and manages the connection to the remote host.
  std::unique_ptr<AppRemotingConnectionHelper> connection_helper_;

  // Called when an ExtensionMessage is received from the host.
  HostMessageReceivedCallback host_message_received_callback_;

  DISALLOW_COPY_AND_ASSIGN(AppRemotingConnectedClientFixture);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_APP_REMOTING_CONNECTED_CLIENT_FIXTURE_H_

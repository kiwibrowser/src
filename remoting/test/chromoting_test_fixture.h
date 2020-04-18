// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_CHROMOTING_TEST_FIXTURE_H_
#define REMOTING_TEST_CHROMOTING_TEST_FIXTURE_H_

#include <memory>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {
namespace test {

class ConnectionTimeObserver;
class TestChromotingClient;

// Provides chromoting connection capabilities for tests to use. Performance
// metrics of the established connection are readily available to calculate
// through the ConnectionTimeObserver.
class ChromotingTestFixture
    : public testing::Test {
 public:
  ChromotingTestFixture();
  ~ChromotingTestFixture() override;

  // Client initiates a session with a host to establish a chromoting
  // connection. Only has |timeout| seconds to connect to host.
  bool ConnectToHost(const base::TimeDelta& timeout);

  // Ends a chromoting connection with the host.
  void Disconnect();

 protected:
  // Observes and saves the times when a chromoting client changes its state.
  std::unique_ptr<ConnectionTimeObserver> connection_time_observer_;

 private:
  // testing::Test overrides.
  void TearDown() override;

  // Creates |connection_time_observer_| to attach to |test_chromoting_client_|.
  void CreateObserver();

  // Detaches |connection_time_observer_| and destroys it.
  void DestroyObserver();

  // Creates and manages the connection to the remote host.
  std::unique_ptr<TestChromotingClient> test_chromoting_client_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(ChromotingTestFixture);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_CHROMOTING_TEST_FIXTURE_H_

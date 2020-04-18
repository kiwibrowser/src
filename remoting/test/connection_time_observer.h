// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_TEST_CONNECTION_TIME_OBSERVER_H_
#define REMOTING_TEST_CONNECTION_TIME_OBSERVER_H_

#include <map>

#include "base/macros.h"
#include "remoting/test/remote_connection_observer.h"

namespace base {
class TimeDelta;
}

namespace remoting {
namespace test {

// Observes and saves the times when a chromoting client changes its state. It
// allows for tests to access latency times between the different states the
// client transitioned through.
class ConnectionTimeObserver
    : public RemoteConnectionObserver {
 public:
  ConnectionTimeObserver();
  ~ConnectionTimeObserver() override;

  // RemoteConnectionObserver interface.
  void ConnectionStateChanged(protocol::ConnectionToHost::State state,
                              protocol::ErrorCode error_code) override;

  // Prints out connection performance stats to STDOUT.
  void DisplayConnectionStats() const;

  // Returns the time delta in milliseconds between |start_state| and
  // |end_state| stored in |transition_times_map_|.
  base::TimeDelta GetStateTransitionTime(
      protocol::ConnectionToHost::State start,
      protocol::ConnectionToHost::State end) const;

  // Used to set fake state transition times for ConnectionTimeObserver tests.
  void SetTransitionTimesMapForTest(
      const std::map<protocol::ConnectionToHost::State, base::TimeTicks>& map);

 private:
  // Saves the current connection state of client to host.
  protocol::ConnectionToHost::State current_connection_state_ =
      protocol::ConnectionToHost::State::INITIALIZING;

  // The TimeTicks to get to a state from the previous state.
  std::map<protocol::ConnectionToHost::State, base::TimeTicks>
      transition_times_map_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionTimeObserver);
};

}  // namespace test
}  // namespace remoting

#endif  // REMOTING_TEST_CONNECTION_TIME_OBSERVER_H_

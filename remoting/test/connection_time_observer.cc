// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/connection_time_observer.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

namespace remoting {
namespace test {

ConnectionTimeObserver::ConnectionTimeObserver() = default;

ConnectionTimeObserver::~ConnectionTimeObserver() = default;

void ConnectionTimeObserver::SetTransitionTimesMapForTest(
    const std::map<protocol::ConnectionToHost::State, base::TimeTicks>& map) {
  transition_times_map_ = map;
}

void ConnectionTimeObserver::ConnectionStateChanged(
    protocol::ConnectionToHost::State state,
    protocol::ErrorCode error_code) {
  if (transition_times_map_.find(state) != transition_times_map_.end()) {
    std::string connection_state =
        protocol::ConnectionToHost::StateToString(state);
    LOG(ERROR) << connection_state << " state has already been set";
    return;
  }
  transition_times_map_.insert(std::make_pair(state, base::TimeTicks::Now()));
  current_connection_state_ = state;
}

void ConnectionTimeObserver::DisplayConnectionStats() const {
  protocol::ConnectionToHost::State initializing =
      protocol::ConnectionToHost::State::INITIALIZING;
  protocol::ConnectionToHost::State current_state = initializing;

  const char kStateChangeTitleFormatString[] = "%-35s%-15s";
  LOG(INFO) << base::StringPrintf(kStateChangeTitleFormatString,
      "State to State", "Delta Time");
  LOG(INFO) << base::StringPrintf(kStateChangeTitleFormatString,
      "--------------", "----------");

  // Note: the order of |connected_states| mimics the expected order of when a
  // connection is made.
  std::vector<protocol::ConnectionToHost::State> connected_states;
  connected_states.push_back(protocol::ConnectionToHost::State::CONNECTING);
  connected_states.push_back(protocol::ConnectionToHost::State::AUTHENTICATED);
  connected_states.push_back(protocol::ConnectionToHost::State::CONNECTED);
  connected_states.push_back(protocol::ConnectionToHost::State::FAILED);

  const char kStateChangeFormatString[] = "%-13s to %-18s%-7dms";
  auto iter_end = transition_times_map_.end();
  for (protocol::ConnectionToHost::State state : connected_states) {
    auto iter_state = transition_times_map_.find(state);
    if (iter_state != iter_end) {
      int state_transition_time =
          GetStateTransitionTime(current_state, state).InMilliseconds();
      LOG(INFO) << base::StringPrintf(kStateChangeFormatString,
          protocol::ConnectionToHost::StateToString(current_state),
          protocol::ConnectionToHost::StateToString(state),
          state_transition_time);
      current_state = state;
    }
  }

  int connected_time =
      GetStateTransitionTime(initializing, current_state).InMilliseconds();

  // |current state| will either be FAILED or CONNECTED.
  LOG(INFO) << "Total Connection Duration (INITIALIZING to "
            << protocol::ConnectionToHost::StateToString(current_state) << "): "
            << connected_time << " ms";
}

base::TimeDelta ConnectionTimeObserver::GetStateTransitionTime(
    protocol::ConnectionToHost::State start,
    protocol::ConnectionToHost::State end) const {
  auto iter_end = transition_times_map_.end();

  auto iter_start_state = transition_times_map_.find(start);
  std::string start_state = protocol::ConnectionToHost::StateToString(start);
  if (iter_start_state == iter_end) {
    LOG(ERROR) << "No time found for state: " << start_state;
    return base::TimeDelta::Max();
  }

  auto iter_end_state = transition_times_map_.find(end);
  std::string end_state = protocol::ConnectionToHost::StateToString(end);
  if (iter_end_state == iter_end) {
    LOG(ERROR) << "No time found for state: " << end_state;
    return base::TimeDelta::Max();
  }

  base::TimeDelta delta = iter_end_state->second - iter_start_state->second;
  if (delta.InMilliseconds() < 0) {
    LOG(ERROR) << "Transition delay is negative. Check the state ordering: "
               << "[start: " << start_state << ", end: " << end_state << "]";
    return base::TimeDelta::Max();
  }

  return delta;
}

}  // namespace test
}  // namespace remoting

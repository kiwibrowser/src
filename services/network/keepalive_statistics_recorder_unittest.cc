// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/keepalive_statistics_recorder.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace {

TEST(KeepaliveStatisticsRecorderTest, InitialState) {
  KeepaliveStatisticsRecorder r;

  EXPECT_EQ(0, r.num_inflight_requests());
  EXPECT_EQ(0, r.peak_inflight_requests());
  EXPECT_TRUE(r.per_process_records().empty());
}

TEST(KeepaliveStatisticsRecorderTest, Register) {
  KeepaliveStatisticsRecorder r;
  constexpr int process_id = 4;
  r.Register(process_id);
  EXPECT_EQ(0, r.num_inflight_requests());
  EXPECT_EQ(0, r.peak_inflight_requests());

  const auto& map = r.per_process_records();
  EXPECT_EQ(1u, map.size());
  auto it = map.find(process_id);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(1, it->second.num_registrations);
  EXPECT_EQ(0, it->second.num_inflight_requests);
  EXPECT_EQ(0, it->second.peak_inflight_requests);
}

TEST(KeepaliveStatisticsRecorderTest, Unregister) {
  KeepaliveStatisticsRecorder r;
  constexpr int process_id = 4;
  r.Register(process_id);
  EXPECT_FALSE(r.per_process_records().empty());
  r.Unregister(process_id);
  EXPECT_TRUE(r.per_process_records().empty());
}

TEST(KeepaliveStatisticsRecorderTest, MultipleRegistration) {
  KeepaliveStatisticsRecorder r;
  constexpr int process1 = 4;
  constexpr int process2 = 7;
  constexpr int process3 = 8;

  r.Register(process1);
  r.Register(process2);
  r.Register(process3);
  r.Register(process1);
  r.Register(process2);

  r.Unregister(process1);
  r.Unregister(process3);

  const auto& map = r.per_process_records();
  EXPECT_EQ(2u, map.size());
  auto it1 = map.find(process1);
  auto it2 = map.find(process2);
  auto it3 = map.find(process3);

  EXPECT_NE(it1, map.end());
  EXPECT_EQ(1, it1->second.num_registrations);
  EXPECT_EQ(0, it1->second.num_inflight_requests);
  EXPECT_EQ(0, it1->second.peak_inflight_requests);
  EXPECT_NE(it2, map.end());
  EXPECT_EQ(2, it2->second.num_registrations);
  EXPECT_EQ(0, it2->second.num_inflight_requests);
  EXPECT_EQ(0, it2->second.peak_inflight_requests);
  EXPECT_EQ(it3, map.end());
}

TEST(KeepaliveStatisticsRecorderTest, IssueOneRequest) {
  KeepaliveStatisticsRecorder r;
  constexpr int process = 4;

  r.Register(process);
  r.OnLoadStarted(process);
  {
    const auto& map = r.per_process_records();
    EXPECT_EQ(1u, map.size());
    auto it = map.find(process);
    ASSERT_NE(it, map.end());
    EXPECT_EQ(1, it->second.num_registrations);
    EXPECT_EQ(1, it->second.num_inflight_requests);
    EXPECT_EQ(1, it->second.peak_inflight_requests);

    EXPECT_EQ(1, r.num_inflight_requests());
    EXPECT_EQ(1, r.peak_inflight_requests());
  }

  r.OnLoadFinished(process);
  {
    const auto& map = r.per_process_records();
    EXPECT_EQ(1u, map.size());
    auto it = map.find(process);
    ASSERT_NE(it, map.end());
    EXPECT_EQ(1, it->second.num_registrations);
    EXPECT_EQ(0, it->second.num_inflight_requests);
    EXPECT_EQ(1, it->second.peak_inflight_requests);

    EXPECT_EQ(0, r.num_inflight_requests());
    EXPECT_EQ(1, r.peak_inflight_requests());
  }
}

TEST(KeepaliveStatisticsRecorderTest, IssueRequests) {
  KeepaliveStatisticsRecorder r;
  constexpr int process1 = 2;
  constexpr int process2 = 3;
  constexpr int no_process = 0;

  r.Register(process1);
  r.Register(process1);
  r.Register(process1);
  r.Register(process2);
  r.Register(process2);

  r.OnLoadStarted(process1);
  r.OnLoadStarted(process1);
  r.OnLoadStarted(process2);
  r.OnLoadStarted(process2);
  r.OnLoadStarted(process2);
  r.OnLoadStarted(process2);
  r.OnLoadStarted(no_process);
  r.OnLoadFinished(process2);
  r.OnLoadFinished(process2);
  r.OnLoadFinished(process2);
  r.OnLoadStarted(process2);
  r.OnLoadStarted(no_process);
  r.OnLoadStarted(no_process);
  r.OnLoadStarted(no_process);
  r.OnLoadStarted(no_process);
  r.OnLoadStarted(no_process);
  r.OnLoadFinished(no_process);

  const auto& map = r.per_process_records();
  EXPECT_EQ(2u, map.size());
  auto it1 = map.find(process1);
  auto it2 = map.find(process2);
  ASSERT_NE(it1, map.end());
  EXPECT_EQ(3, it1->second.num_registrations);
  EXPECT_EQ(2, it1->second.num_inflight_requests);
  EXPECT_EQ(2, it1->second.peak_inflight_requests);

  ASSERT_NE(it2, map.end());
  EXPECT_EQ(2, it2->second.num_registrations);
  EXPECT_EQ(2, it2->second.num_inflight_requests);
  EXPECT_EQ(4, it2->second.peak_inflight_requests);

  EXPECT_EQ(9, r.num_inflight_requests());
  EXPECT_EQ(10, r.peak_inflight_requests());
}

TEST(KeepaliveStatisticsRecorderTest, ProcessReuse) {
  KeepaliveStatisticsRecorder r;
  constexpr int process = 2;

  r.Register(process);
  r.OnLoadStarted(process);
  r.OnLoadStarted(process);
  r.OnLoadStarted(process);
  r.OnLoadFinished(process);
  r.OnLoadFinished(process);
  r.OnLoadFinished(process);
  r.Unregister(process);

  r.Register(process);
  const auto& map = r.per_process_records();
  EXPECT_EQ(1u, map.size());
  auto it = map.find(process);
  ASSERT_NE(it, map.end());
  EXPECT_EQ(1, it->second.num_registrations);
  EXPECT_EQ(0, it->second.num_inflight_requests);
  EXPECT_EQ(0, it->second.peak_inflight_requests);
}

}  // namespace

}  // namespace network

// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/ssrc.h"

#include <vector>

#include "osp_base/std_util.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace cast_streaming {
namespace {

TEST(SsrcTest, GeneratesUniqueAndPrioritizedSsrcs) {
  std::vector<Ssrc> priority_ssrcs;
  for (int i = 0; i < 3; ++i) {
    priority_ssrcs.push_back(GenerateSsrc(true));
  }

  // Three different higher-priority SSRCs should have been generated.
  SortAndDedupeElements(&priority_ssrcs);
  EXPECT_EQ(3u, priority_ssrcs.size());

  std::vector<Ssrc> normal_ssrcs;
  for (int i = 0; i < 3; ++i) {
    normal_ssrcs.push_back(GenerateSsrc(false));
  }

  // Three different normal SSRCs should have been generated.
  SortAndDedupeElements(&normal_ssrcs);
  EXPECT_EQ(3u, normal_ssrcs.size());

  // All six SSRCs, together, should be unique.
  std::vector<Ssrc> all_ssrcs;
  all_ssrcs.insert(all_ssrcs.end(), priority_ssrcs.begin(),
                   priority_ssrcs.end());
  all_ssrcs.insert(all_ssrcs.end(), normal_ssrcs.begin(), normal_ssrcs.end());
  SortAndDedupeElements(&all_ssrcs);
  EXPECT_EQ(6u, all_ssrcs.size());

  // ComparePriority() should return values indicating the appropriate
  // prioritization.
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      EXPECT_LT(ComparePriority(priority_ssrcs[i], normal_ssrcs[j]), 0);
      EXPECT_GT(ComparePriority(normal_ssrcs[i], priority_ssrcs[j]), 0);
    }
  }
}

}  // namespace
}  // namespace cast_streaming
}  // namespace openscreen

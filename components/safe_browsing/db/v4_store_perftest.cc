// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/numerics/checked_math.h"
#include "base/strings/stringprintf.h"
#include "base/test/test_simple_task_runner.h"
#include "base/timer/elapsed_timer.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "components/safe_browsing/db/v4_test_util.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"

namespace safe_browsing {

class V4StorePerftest : public testing::Test {};

TEST_F(V4StorePerftest, StressTest) {
// Debug builds can be quite slow. Use a smaller number of prefixes to test.
#if defined(NDEBUG)
  const size_t kNumPrefixes = 2000000;
#else
  const size_t kNumPrefixes = 20000;
#endif

  static_assert(kMaxHashPrefixLength == crypto::kSHA256Length,
                "SHA256 produces a valid FullHash");
  CHECK(base::IsValidForType<size_t>(
      base::CheckMul(kNumPrefixes, kMaxHashPrefixLength)));

  // Keep the full hashes as one big string to avoid tons of allocations /
  // deallocations in the test.
  std::string full_hashes(kNumPrefixes * kMaxHashPrefixLength, 0);
  base::StringPiece full_hashes_piece = base::StringPiece(full_hashes);
  std::vector<std::string> prefixes;
  for (size_t i = 0; i < kNumPrefixes; i++) {
    size_t index = i * kMaxHashPrefixLength;
    crypto::SHA256HashString(base::StringPrintf("%zu", i), &full_hashes[index],
                             kMaxHashPrefixLength);
    prefixes.push_back(full_hashes.substr(index, kMinHashPrefixLength));
  }

  auto store = std::make_unique<TestV4Store>(
      base::MakeRefCounted<base::TestSimpleTaskRunner>(), base::FilePath());
  store->SetPrefixes(std::move(prefixes), kMinHashPrefixLength);

  size_t matches = 0;
  base::ElapsedTimer timer;
  for (size_t i = 0; i < kNumPrefixes; i++) {
    size_t index = i * kMaxHashPrefixLength;
    base::StringPiece full_hash =
        full_hashes_piece.substr(index, kMaxHashPrefixLength);
    matches += !store->GetMatchingHashPrefix(full_hash).empty();
  }
  perf_test::PrintResult("GetMatchingHashPrefix", "", "",
                         timer.Elapsed().InMillisecondsF(), "ms", true);

  EXPECT_EQ(kNumPrefixes, matches);
}

}  // namespace safe_browsing

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webdatabase/quota_tracker.h"

#include "base/memory/scoped_refptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {
namespace {

TEST(QuotaTrackerTest, UpdateAndGetSizeAndSpaceAvailable) {
  QuotaTracker& tracker = QuotaTracker::Instance();
  scoped_refptr<const SecurityOrigin> origin =
      SecurityOrigin::CreateFromString("file:///a/b/c");

  const String database_name = "db";
  const unsigned long long kDatabaseSize = 1234ULL;
  tracker.UpdateDatabaseSize(origin.get(), database_name, kDatabaseSize);

  unsigned long long used = 0;
  unsigned long long available = 0;
  tracker.GetDatabaseSizeAndSpaceAvailableToOrigin(origin.get(), database_name,
                                                   &used, &available);

  EXPECT_EQ(used, kDatabaseSize);
  EXPECT_EQ(available, 0UL);
}

TEST(QuotaTrackerTest, LocalAccessBlocked) {
  QuotaTracker& tracker = QuotaTracker::Instance();
  scoped_refptr<SecurityOrigin> origin =
      SecurityOrigin::CreateFromString("file:///a/b/c");

  const String database_name = "db";
  const unsigned long long kDatabaseSize = 1234ULL;
  tracker.UpdateDatabaseSize(origin.get(), database_name, kDatabaseSize);

  // QuotaTracker should not care about policy, just identity.
  origin->BlockLocalAccessFromLocalOrigin();

  unsigned long long used = 0;
  unsigned long long available = 0;
  tracker.GetDatabaseSizeAndSpaceAvailableToOrigin(origin.get(), database_name,
                                                   &used, &available);

  EXPECT_EQ(used, kDatabaseSize);
  EXPECT_EQ(available, 0UL);
}

}  // namespace
}  // namespace blink

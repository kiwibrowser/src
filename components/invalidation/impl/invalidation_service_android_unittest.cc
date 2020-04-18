// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/invalidation_service_android.h"

#include "build/build_config.h"
#include "components/invalidation/impl/fake_invalidation_handler.h"
#include "components/invalidation/impl/invalidation_service_test_template.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace invalidation {

#if defined(OS_ANDROID)

class InvalidationServiceAndroidTest : public testing::Test {
 public:
  InvalidationServiceAndroidTest() : invalidation_service_() {}
  ~InvalidationServiceAndroidTest() override {}

  InvalidationService& invalidation_service() {
    return invalidation_service_;
  }

 private:
  InvalidationServiceAndroid invalidation_service_;
};

TEST_F(InvalidationServiceAndroidTest, FetchClientId) {
  const std::string id1 = invalidation_service().GetInvalidatorClientId();
  ASSERT_FALSE(id1.empty());

  // If nothing else, the ID should be consistent.
  const std::string id2 = invalidation_service().GetInvalidatorClientId();
  ASSERT_EQ(id1, id2);
}

#endif

}  // namespace invalidation

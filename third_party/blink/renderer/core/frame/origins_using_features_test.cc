// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/hosts_using_features.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(HostsUsingFeaturesTest, countName) {
  HostsUsingFeatures hosts_using_features;
  hosts_using_features.CountName(HostsUsingFeatures::Feature::kEventPath,
                                 "test 1");
  EXPECT_EQ(1u, hosts_using_features.ValueByName().size());
  hosts_using_features.CountName(
      HostsUsingFeatures::Feature::kElementCreateShadowRoot, "test 1");
  EXPECT_EQ(1u, hosts_using_features.ValueByName().size());
  hosts_using_features.CountName(HostsUsingFeatures::Feature::kEventPath,
                                 "test 2");
  EXPECT_EQ(2u, hosts_using_features.ValueByName().size());

  EXPECT_TRUE(hosts_using_features.ValueByName().at("test 1").Get(
      HostsUsingFeatures::Feature::kEventPath));
  EXPECT_TRUE(hosts_using_features.ValueByName().at("test 1").Get(
      HostsUsingFeatures::Feature::kElementCreateShadowRoot));
  EXPECT_FALSE(hosts_using_features.ValueByName().at("test 1").Get(
      HostsUsingFeatures::Feature::kDocumentRegisterElement));
  EXPECT_TRUE(hosts_using_features.ValueByName().at("test 2").Get(
      HostsUsingFeatures::Feature::kEventPath));
  EXPECT_FALSE(hosts_using_features.ValueByName().at("test 2").Get(
      HostsUsingFeatures::Feature::kElementCreateShadowRoot));
  EXPECT_FALSE(hosts_using_features.ValueByName().at("test 2").Get(
      HostsUsingFeatures::Feature::kDocumentRegisterElement));

  hosts_using_features.Clear();
}

}  // namespace blink

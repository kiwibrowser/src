// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/media_router_feature.h"

#include "base/test/scoped_feature_list.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

TEST(MediaRouterFeatureTest, GetCastAllowAllIPsPref) {
  auto pref_service = std::make_unique<TestingPrefServiceSimple>();
  pref_service->registry()->RegisterBooleanPref(
      prefs::kMediaRouterCastAllowAllIPs, false);
  EXPECT_FALSE(GetCastAllowAllIPsPref(pref_service.get()));

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(kCastAllowAllIPsFeature);
  EXPECT_TRUE(GetCastAllowAllIPsPref(pref_service.get()));

  pref_service->SetManagedPref(prefs::kMediaRouterCastAllowAllIPs,
                               std::make_unique<base::Value>(true));
  EXPECT_TRUE(GetCastAllowAllIPsPref(pref_service.get()));

  pref_service->SetManagedPref(prefs::kMediaRouterCastAllowAllIPs,
                               std::make_unique<base::Value>(false));
  EXPECT_FALSE(GetCastAllowAllIPsPref(pref_service.get()));
}

}  // namespace media_router

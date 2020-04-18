// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash_service_registry.h"

#include "ash/public/interfaces/constants.mojom.h"
#include "base/stl_util.h"
#include "base/test/scoped_feature_list.h"
#include "content/public/browser/content_browser_client.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/ui_base_features.h"

TEST(AshServiceRegistryTest, AshAndUiInSameProcess) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kMash);
  content::ContentBrowserClient::OutOfProcessServiceMap services;
  ash_service_registry::RegisterOutOfProcessServices(&services);

  // The ash service and ui service should be in the same process group.
  ASSERT_TRUE(base::ContainsKey(services, ash::mojom::kServiceName));
  ASSERT_TRUE(base::ContainsKey(services, ui::mojom::kServiceName));
  std::string ash_process_group =
      *services[ash::mojom::kServiceName].process_group;
  std::string ui_process_group =
      *services[ui::mojom::kServiceName].process_group;
  EXPECT_FALSE(ash_process_group.empty());
  EXPECT_FALSE(ui_process_group.empty());
  EXPECT_EQ(ash_process_group, ui_process_group);
}

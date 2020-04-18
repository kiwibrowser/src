// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/values.h"
#include "chrome/browser/media/router/media_router_factory.h"
#include "chrome/browser/media/router/test/mock_media_router.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model_factory.h"
#include "chrome/browser/ui/webui/media_router/media_router_ui_service.h"
#include "chrome/browser/ui/webui/media_router/media_router_ui_service_factory.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

class MediaRouterUIServiceFactoryUnitTest : public testing::Test {
 public:
  MediaRouterUIServiceFactoryUnitTest() {}
  ~MediaRouterUIServiceFactoryUnitTest() override {}

  void SetUp() override {
    TestingProfile::Builder builder;
    // MediaRouterUIService instantiates MediaRouterActionController, which
    // requires ToolbarActionsModel.
    builder.AddTestingFactory(ToolbarActionsModelFactory::GetInstance(),
                              BuildFakeToolBarActionsModel);
    builder.AddTestingFactory(MediaRouterFactory::GetInstance(),
                              MockMediaRouter::Create);
    profile_ = builder.Build();
  }

  static std::unique_ptr<KeyedService> BuildFakeToolBarActionsModel(
      content::BrowserContext* context) {
    return std::unique_ptr<ToolbarActionsModel>(
        new ToolbarActionsModel(static_cast<Profile*>(context), nullptr));
  }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(MediaRouterUIServiceFactoryUnitTest, CreateService) {
  // We call BuildServiceInstanceFor() directly because
  // MediaRouterUIServiceFactory::GetForBrowserContext() is set to return a
  // nullptr for a test profile.
  std::unique_ptr<MediaRouterUIService> service(
      static_cast<MediaRouterUIService*>(
          MediaRouterUIServiceFactory::GetInstance()->BuildServiceInstanceFor(
              profile_.get())));
  ASSERT_TRUE(service);
  ASSERT_TRUE(service->action_controller());
}

TEST_F(MediaRouterUIServiceFactoryUnitTest, DoNotCreateServiceWhenDisabled) {
  profile_->GetTestingPrefService()->SetManagedPref(
      prefs::kEnableMediaRouter, std::make_unique<base::Value>(false));
  EXPECT_EQ(nullptr,
            MediaRouterUIServiceFactory::GetInstance()->BuildServiceInstanceFor(
                profile_.get()));
}

}  // namespace media_router

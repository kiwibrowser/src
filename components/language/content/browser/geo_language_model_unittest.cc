// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/content/browser/geo_language_model.h"

#include "base/macros.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/timer/timer.h"
#include "components/language/content/browser/geo_language_provider.h"
#include "components/language/content/browser/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace language {
namespace {

// Compares LanguageDetails.
MATCHER_P(EqualsLd, lang_details, "") {
  constexpr static float kFloatEps = 0.00001f;
  return arg.lang_code == lang_details.lang_code &&
         std::abs(arg.score - lang_details.score) < kFloatEps;
}

}  // namespace

class GeoLanguageModelTest : public testing::Test {
 public:
  GeoLanguageModelTest()
      : task_runner_(base::MakeRefCounted<base::TestMockTimeTaskRunner>(
            base::TestMockTimeTaskRunner::Type::kBoundToThread)),
        scoped_context_(task_runner_.get()),
        geo_language_provider_(task_runner_),
        geo_language_model_(&geo_language_provider_),
        mock_ip_geo_location_provider_(&mock_geo_location_) {
    service_manager::mojom::ConnectorRequest request;
    connector_ = service_manager::Connector::Create(&request);
    service_manager::Connector::TestApi test_api(connector_.get());
    test_api.OverrideBinderForTesting(
        service_manager::Identity(device::mojom::kServiceName),
        device::mojom::PublicIpAddressGeolocationProvider::Name_,
        base::BindRepeating(&MockIpGeoLocationProvider::Bind,
                            base::Unretained(&mock_ip_geo_location_provider_)));
  }

 protected:
  void StartGeoLanguageProvider() {
    geo_language_provider_.StartUp(std::move(connector_));
  }

  void MoveToLocation(float latitude, float longitude) {
    mock_geo_location_.MoveToLocation(latitude, longitude);
  }

  const scoped_refptr<base::TestMockTimeTaskRunner>& GetTaskRunner() {
    return task_runner_;
  }

  GeoLanguageModel* language_model() { return &geo_language_model_; }

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  const base::TestMockTimeTaskRunner::ScopedContext scoped_context_;

  GeoLanguageProvider geo_language_provider_;
  // Object under test.
  GeoLanguageModel geo_language_model_;
  MockGeoLocation mock_geo_location_;
  MockIpGeoLocationProvider mock_ip_geo_location_provider_;
  std::unique_ptr<service_manager::Connector> connector_;
};

TEST_F(GeoLanguageModelTest, InsideIndia) {
  // Setup a random place in Madhya Pradesh, India.
  MoveToLocation(23.0, 80.0);
  StartGeoLanguageProvider();
  const auto task_runner = GetTaskRunner();
  task_runner->RunUntilIdle();

  EXPECT_THAT(language_model()->GetLanguages(),
              testing::ElementsAre(
                  EqualsLd(LanguageModel::LanguageDetails("hi", 0.f)),
                  EqualsLd(LanguageModel::LanguageDetails("mr", 0.f)),
                  EqualsLd(LanguageModel::LanguageDetails("ur", 0.f))));
}

TEST_F(GeoLanguageModelTest, OutsideIndia) {
  // Setup a random place outside of India.
  MoveToLocation(0.0, 0.0);
  StartGeoLanguageProvider();
  const auto task_runner = GetTaskRunner();
  task_runner->RunUntilIdle();

  EXPECT_EQ(0UL, language_model()->GetLanguages().size());
}

}  // namespace language

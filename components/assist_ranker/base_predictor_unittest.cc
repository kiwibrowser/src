// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/assist_ranker/base_predictor.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "components/assist_ranker/fake_ranker_model_loader.h"
#include "components/assist_ranker/predictor_config.h"
#include "components/assist_ranker/proto/ranker_example.pb.h"
#include "components/assist_ranker/ranker_model.h"
#include "components/ukm/test_ukm_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace assist_ranker {

using ::assist_ranker::testing::FakeRankerModelLoader;

namespace {

// Predictor config for testing.
const char kTestModelName[] = "test_model";
// This name needs to be an entry in ukm.xml
const char kTestLoggingName[] = "ContextualSearch";
const char kTestUmaPrefixName[] = "Test.Ranker";
const char kTestUrlParamName[] = "ranker-model-url";
const char kTestDefaultModelUrl[] = "https://foo.bar/model.bin";

// The whitelisted features must be metrics of kTestLoggingName in ukm.xml,
// though the types do not need to match.
const char kBoolFeature[] = "DidOptIn";
const char kIntFeature[] = "DurationAfterScrollMs";
const char kFloatFeature[] = "FontSize";
const char kStringFeature[] = "IsEntity";
const char kStringListFeature[] = "IsEntityEligible";
const char kFeatureNotWhitelisted[] = "not_whitelisted";

const char kTestNavigationUrl[] = "https://foo.com";

const base::flat_set<std::string> kFeatureWhitelist({kBoolFeature, kIntFeature,
                                                     kFloatFeature,
                                                     kStringFeature,
                                                     kStringListFeature});

const base::Feature kTestRankerQuery{"TestRankerQuery",
                                     base::FEATURE_ENABLED_BY_DEFAULT};

const base::FeatureParam<std::string> kTestRankerUrl{
    &kTestRankerQuery, kTestUrlParamName, kTestDefaultModelUrl};

const PredictorConfig kTestPredictorConfig = PredictorConfig{
    kTestModelName,     kTestLoggingName,  kTestUmaPrefixName, LOG_UKM,
    &kFeatureWhitelist, &kTestRankerQuery, &kTestRankerUrl};

// Class that implements virtual functions of the base class.
class FakePredictor : public BasePredictor {
 public:
  static std::unique_ptr<FakePredictor> Create();
  ~FakePredictor() override{};
  // Validation will always succeed.
  static RankerModelStatus ValidateModel(const RankerModel& model);

 protected:
  // Not implementing any inference logic.
  bool Initialize() override { return true; };

 private:
  FakePredictor(const PredictorConfig& config);
  DISALLOW_COPY_AND_ASSIGN(FakePredictor);
};

FakePredictor::FakePredictor(const PredictorConfig& config)
    : BasePredictor(config) {}

RankerModelStatus FakePredictor::ValidateModel(const RankerModel& model) {
  return RankerModelStatus::OK;
}

std::unique_ptr<FakePredictor> FakePredictor::Create() {
  std::unique_ptr<FakePredictor> predictor(
      new FakePredictor(kTestPredictorConfig));
  auto ranker_model = std::make_unique<RankerModel>();
  auto fake_model_loader = std::make_unique<FakeRankerModelLoader>(
      base::BindRepeating(&FakePredictor::ValidateModel),
      base::BindRepeating(&FakePredictor::OnModelAvailable,
                          base::Unretained(predictor.get())),
      std::move(ranker_model));
  predictor->LoadModel(std::move(fake_model_loader));
  return predictor;
}

}  // namespace

class BasePredictorTest : public ::testing::Test {
 protected:
  BasePredictorTest() = default;
  // Disables Query for the test predictor.
  void DisableQuery();

  ukm::SourceId GetSourceId();

  ukm::TestUkmRecorder* GetTestUkmRecorder() { return &test_ukm_recorder_; }

 private:
  // Sets up the task scheduling/task-runner environment for each test.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Sets itself as the global UkmRecorder on construction.
  ukm::TestAutoSetUkmRecorder test_ukm_recorder_;

  // Manages the enabling/disabling of features within the scope of a test.
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(BasePredictorTest);
};

ukm::SourceId BasePredictorTest::GetSourceId() {
  ukm::SourceId source_id = ukm::UkmRecorder::GetNewSourceID();
  test_ukm_recorder_.UpdateSourceURL(source_id, GURL(kTestNavigationUrl));
  return source_id;
}

void BasePredictorTest::DisableQuery() {
  scoped_feature_list_.InitWithFeatures({}, {kTestRankerQuery});
}

TEST_F(BasePredictorTest, BaseTest) {
  auto predictor = FakePredictor::Create();
  EXPECT_EQ(kTestModelName, predictor->GetModelName());
  EXPECT_EQ(kTestDefaultModelUrl, predictor->GetModelUrl());
  EXPECT_TRUE(predictor->is_query_enabled());
  EXPECT_TRUE(predictor->IsReady());
}

TEST_F(BasePredictorTest, QueryDisabled) {
  DisableQuery();
  auto predictor = FakePredictor::Create();
  EXPECT_EQ(kTestModelName, predictor->GetModelName());
  EXPECT_EQ(kTestDefaultModelUrl, predictor->GetModelUrl());
  EXPECT_FALSE(predictor->is_query_enabled());
  EXPECT_FALSE(predictor->IsReady());
}

TEST_F(BasePredictorTest, LogExampleToUkm) {
  auto predictor = FakePredictor::Create();
  RankerExample example;
  auto& features = *example.mutable_features();
  features[kBoolFeature].set_bool_value(true);
  features[kIntFeature].set_int32_value(42);
  features[kFloatFeature].set_float_value(42.0f);
  features[kStringFeature].set_string_value("42");
  features[kStringListFeature].mutable_string_list()->add_string_value("42");

  // This feature will not be logged.
  features[kFeatureNotWhitelisted].set_bool_value(false);

  predictor->LogExampleToUkm(example, GetSourceId());

  EXPECT_EQ(1U, GetTestUkmRecorder()->sources_count());
  EXPECT_EQ(1U, GetTestUkmRecorder()->entries_count());
  std::vector<const ukm::mojom::UkmEntry*> entries =
      GetTestUkmRecorder()->GetEntriesByName(kTestLoggingName);
  EXPECT_EQ(1U, entries.size());
  GetTestUkmRecorder()->ExpectEntryMetric(entries[0], kBoolFeature,
                                          72057594037927937);
  GetTestUkmRecorder()->ExpectEntryMetric(entries[0], kIntFeature,
                                          216172782113783850);
  GetTestUkmRecorder()->ExpectEntryMetric(entries[0], kFloatFeature,
                                          144115189185773568);
  GetTestUkmRecorder()->ExpectEntryMetric(entries[0], kStringFeature,
                                          288230377208836903);
  GetTestUkmRecorder()->ExpectEntryMetric(entries[0], kStringListFeature,
                                          360287971246764839);

  EXPECT_FALSE(
      GetTestUkmRecorder()->EntryHasMetric(entries[0], kFeatureNotWhitelisted));
}

}  // namespace assist_ranker

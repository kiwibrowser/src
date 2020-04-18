// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_ranker/tab_score_predictor.h"

#include <string>

#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/metrics_hashes.h"
#include "chrome/browser/resource_coordinator/tab_ranker/mru_features.h"
#include "chrome/browser/resource_coordinator/tab_ranker/native_inference.h"
#include "chrome/browser/resource_coordinator/tab_ranker/tab_features.h"
#include "chrome/browser/resource_coordinator/tab_ranker/window_features.h"
#include "chrome/grit/browser_resources.h"
#include "components/assist_ranker/example_preprocessing.h"
#include "components/assist_ranker/proto/example_preprocessor.pb.h"
#include "components/assist_ranker/proto/ranker_example.pb.h"
#include "ui/base/resource/resource_bundle.h"

namespace tab_ranker {
namespace {

// Loads the preprocessor config protobuf, which lists each feature, their
// types, bucket configurations, etc.
// Returns true if the protobuf was successfully populated.
std::unique_ptr<assist_ranker::ExamplePreprocessorConfig>
LoadExamplePreprocessorConfig() {
  auto config = std::make_unique<assist_ranker::ExamplePreprocessorConfig>();

  scoped_refptr<base::RefCountedMemory> raw_config =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
          IDR_TAB_RANKER_EXAMPLE_PREPROCESSOR_CONFIG_PB);
  if (!raw_config || !raw_config->front()) {
    LOG(ERROR) << "Failed to load TabRanker example preprocessor config.";
    return nullptr;
  }

  if (!config->ParseFromArray(raw_config->front(), raw_config->size())) {
    LOG(ERROR) << "Failed to parse TabRanker example preprocessor config.";
    return nullptr;
  }

  return config;
}

void PopulateRankerExample(assist_ranker::RankerExample* example,
                           const TabFeatures& tab,
                           const WindowFeatures& window,
                           const MRUFeatures& mru) {
  auto& features = *example->mutable_features();

  features["HasBeforeUnloadHandler"].set_bool_value(
      tab.has_before_unload_handler);
  features["HasFormEntry"].set_bool_value(tab.has_form_entry);
  features["IsPinned"].set_bool_value(tab.is_pinned);
  features["KeyEventCount"].set_int32_value(tab.key_event_count);
  features["MRUIndex"].set_int32_value(mru.index);
  features["MouseEventCount"].set_int32_value(tab.mouse_event_count);
  features["NavigationEntryCount"].set_int32_value(tab.navigation_entry_count);
  DCHECK_GT(mru.total, 0);
  features["NormalizedMRUIndex"].set_float_value(float(mru.index) / mru.total);
  features["NumReactivationBefore"].set_int32_value(tab.num_reactivations);

  // Nullable types indicate optional values; if not present, the corresponding
  // feature should not be set.
  if (tab.page_transition_core_type.has_value()) {
    features["PageTransitionCoreType"].set_int32_value(
        tab.page_transition_core_type.value());
  }
  features["PageTransitionFromAddressBar"].set_bool_value(
      tab.page_transition_from_address_bar);
  features["PageTransitionIsRedirect"].set_bool_value(
      tab.page_transition_is_redirect);
  if (tab.site_engagement_score.has_value()) {
    features["SiteEngagementScore"].set_int32_value(
        tab.site_engagement_score.value());
  }
  features["TabCount"].set_int32_value(window.tab_count);
  features["TimeFromBackgrounded"].set_int32_value(tab.time_from_backgrounded);
  features["TopDomain"].set_string_value(
      std::to_string(base::HashMetricName(tab.host)));
  features["TotalTabCount"].set_int32_value(mru.total);
  features["TouchEventCount"].set_int32_value(tab.touch_event_count);
  features["Type"].set_int32_value(window.type);
  features["WasRecentlyAudible"].set_bool_value(tab.was_recently_audible);
}

}  // namespace

TabScorePredictor::TabScorePredictor() = default;
TabScorePredictor::~TabScorePredictor() = default;

TabRankerResult TabScorePredictor::ScoreTab(const TabFeatures& tab,
                                            const WindowFeatures& window,
                                            const MRUFeatures& mru,
                                            float* score) {
  DCHECK(score);

  // No error is expected, but something could conceivably be misconfigured.
  TabRankerResult result = TabRankerResult::kSuccess;

  // Lazy-load the preprocessor config.
  LazyInitialize();
  if (preprocessor_config_) {
    // Build the RankerExample using the tab's features.
    assist_ranker::RankerExample example;
    PopulateRankerExample(&example, tab, window, mru);

    // Process the RankerExample with the tab ranker config to vectorize the
    // feature list for inference.
    int preprocessor_error = preprocessor_->Process(&example);
    if (preprocessor_error) {
      // kNoFeatureIndexFound can occur normally (e.g., when the domain name
      // isn't known to the model or a rarely seen enum value is used).
      DCHECK_EQ(assist_ranker::ExamplePreprocessor::kNoFeatureIndexFound,
                preprocessor_error);
    }

    // This vector will be provided to the inference function.
    const auto& vectorized_features =
        example.features()
            .at(assist_ranker::ExamplePreprocessor::
                    kVectorizedFeatureDefaultName)
            .float_list()
            .float_value();
    CHECK_EQ(vectorized_features.size(), tfnative_model::FEATURES_SIZE);

    // Fixed amount of memory the inference function will use.
    if (!model_alloc_)
      model_alloc_ = std::make_unique<tfnative_model::FixedAllocations>();
    tfnative_model::Inference(vectorized_features.data(), score,
                              model_alloc_.get());

    if (preprocessor_error &&
        preprocessor_error !=
            assist_ranker::ExamplePreprocessor::kNoFeatureIndexFound) {
      // May indicate something is wrong with how we create the RankerExample.
      result = TabRankerResult::kPreprocessorOtherError;
    }
  } else {
    result = TabRankerResult::kPreprocessorInitializationFailed;
  }

  UMA_HISTOGRAM_ENUMERATION("TabManager.TabRanker.Result", result);
  return result;
}

void TabScorePredictor::LazyInitialize() {
  if (preprocessor_config_)
    return;

  preprocessor_config_ = LoadExamplePreprocessorConfig();
  if (preprocessor_config_) {
    preprocessor_ = std::make_unique<assist_ranker::ExamplePreprocessor>(
        *preprocessor_config_);
  }
}

}  // namespace tab_ranker

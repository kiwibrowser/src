// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREDICTORS_LOADING_PREDICTOR_CONFIG_H_
#define CHROME_BROWSER_PREDICTORS_LOADING_PREDICTOR_CONFIG_H_

#include <cstddef>

#include "base/feature_list.h"

class Profile;

namespace predictors {

extern const char kSpeculativePreconnectFeatureName[];
extern const char kModeParamName[];
extern const char kLearningMode[];
extern const char kPreconnectMode[];
extern const char kNoPreconnectMode[];
extern const base::Feature kSpeculativePreconnectFeature;

struct LoadingPredictorConfig;

// Returns whether the predictor is enabled, and populates |config|, if not
// nullptr.
bool IsLoadingPredictorEnabled(Profile* profile,
                               LoadingPredictorConfig* config);

// Returns true if speculative preconnect is enabled, and initializes |config|,
// if not nullptr.
bool MaybeEnableSpeculativePreconnect(LoadingPredictorConfig* config);

// Returns true if all other implementations of preconnect should be disabled.
bool ShouldDisableOtherPreconnects();

// Indicates what caused the page load hint.
enum class HintOrigin { NAVIGATION, EXTERNAL, OMNIBOX };

// Represents the config for the Loading predictor.
struct LoadingPredictorConfig {
  // Initializes the config with default values.
  LoadingPredictorConfig();
  LoadingPredictorConfig(const LoadingPredictorConfig& other);
  ~LoadingPredictorConfig();

  // The mode the LoadingPredictor is running in. Forms a bit map.
  enum Mode {
    LEARNING = 1 << 0,
    PREFETCHING_FOR_NAVIGATION = 1 << 2,  // deprecated
    PREFETCHING_FOR_EXTERNAL = 1 << 3,    // deprecated
    PRECONNECT = 1 << 4
  };
  int mode;

  // Helpers to deal with mode.
  bool IsLearningEnabled() const;
  bool IsPreconnectEnabledForSomeOrigin(Profile* profile) const;
  bool IsPreconnectEnabledForOrigin(Profile* profile, HintOrigin origin) const;

  bool IsSmallDBEnabledForTest() const;

  // If a navigation hasn't seen a load complete event in this much time, it
  // is considered abandoned.
  size_t max_navigation_lifetime_seconds;

  // Size of LRU caches for the host data.
  size_t max_hosts_to_track;

  // The maximum number of origins to store per entry.
  size_t max_origins_per_entry;
  // The number of consecutive misses after which we stop tracking a resource
  // URL.
  size_t max_consecutive_misses;
  // The number of consecutive misses after which we stop tracking a redirect
  // endpoint.
  size_t max_redirect_consecutive_misses;

  // True iff origin-based learning is enabled.
  bool is_origin_learning_enabled;

  // True iff all other implementations of preconnect should be disabled.
  bool should_disable_other_preconnects;

  // Delay between writing data to the predictors database memory cache and
  // flushing it to disk.
  size_t flush_data_to_disk_delay_seconds;
};

}  // namespace predictors

#endif  // CHROME_BROWSER_PREDICTORS_LOADING_PREDICTOR_CONFIG_H_

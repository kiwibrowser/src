// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_RANKER_TAB_SCORE_PREDICTOR_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_RANKER_TAB_SCORE_PREDICTOR_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"

namespace assist_ranker {
class ExamplePreprocessor;
class ExamplePreprocessorConfig;
}  // namespace assist_ranker

namespace tab_ranker {

namespace tfnative_model {
struct FixedAllocations;
}  // namespace tfnative_model

struct MRUFeatures;
struct TabFeatures;
struct WindowFeatures;

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class TabRankerResult {
  kSuccess = 0,
  kPreprocessorInitializationFailed = 1,
  kPreprocessorOtherError = 2,
  kMaxValue = kPreprocessorOtherError
};

// Makes predictions using the tab reactivation DNN classifier. Background tabs
// are scored based on how likely they are to be reactivated.
class TabScorePredictor {
 public:
  TabScorePredictor();
  ~TabScorePredictor();

  // Scores the tab using the tab reactivation model. A higher score indicates
  // the tab is more likely to be reactivated than a lower score. A lower score
  // indicates the tab is more likely to be closed.
  // |mru_index|: Index of the tab in most-recently used order.
  // |total_tab_count|: Number of tabs used when calculating mru_index, ie
  // number of non-incognito tabs.
  TabRankerResult ScoreTab(const TabFeatures& tab,
                           const WindowFeatures& window,
                           const MRUFeatures& mru,
                           float* score) WARN_UNUSED_RESULT;

 private:
  // Loads the preprocessor config if not already loaded.
  void LazyInitialize();

  std::unique_ptr<assist_ranker::ExamplePreprocessorConfig>
      preprocessor_config_;
  std::unique_ptr<assist_ranker::ExamplePreprocessor> preprocessor_;

  // Fixed-size working memory provided to the inferencing function. Lazy
  // initialized once so it isn't reallocated for every inference.
  std::unique_ptr<tfnative_model::FixedAllocations> model_alloc_;

  DISALLOW_COPY_AND_ASSIGN(TabScorePredictor);
};

}  // namespace tab_ranker

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_RANKER_TAB_SCORE_PREDICTOR_H_

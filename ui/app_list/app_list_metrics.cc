// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/app_list_metrics.h"

#include "ash/app_list/model/app_list_model.h"
#include "ash/app_list/model/search/search_model.h"
#include "ash/app_list/model/search/search_result.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "ui/compositor/compositor.h"

namespace app_list {

namespace {

int CalculateAnimationSmoothness(int actual_frames,
                                 int ideal_duration_ms,
                                 float refresh_rate) {
  int smoothness = 100;
  const int ideal_frames =
      refresh_rate * ideal_duration_ms / base::Time::kMillisecondsPerSecond;
  if (ideal_frames > actual_frames)
    smoothness = 100 * actual_frames / ideal_frames;
  return smoothness;
}

}  // namespace

// The UMA histogram that logs smoothness of folder show/hide animation.
constexpr char kFolderShowHideAnimationSmoothness[] =
    "Apps.AppListFolder.ShowHide.AnimationSmoothness";

// The UMA histogram that logs smoothness of pagination animation.
constexpr char kPaginationTransitionAnimationSmoothness[] =
    "Apps.PaginationTransition.AnimationSmoothness";

// The UMA histogram that logs which state search results are opened from.
constexpr char kAppListSearchResultOpenSourceHistogram[] =
    "Apps.AppListSearchResultOpenedSource";

// The different sources from which a search result is displayed. These values
// are written to logs.  New enum values can be added, but existing enums must
// never be renumbered or deleted and reused.
enum class ApplistSearchResultOpenedSource {
  kHalfClamshell = 0,
  kFullscreenClamshell = 1,
  kFullscreenTablet = 2,
  kMaxApplistSearchResultOpenedSource = 3,
};

void RecordFolderShowHideAnimationSmoothness(int actual_frames,
                                             int ideal_duration_ms,
                                             float refresh_rate) {
  const int smoothness = CalculateAnimationSmoothness(
      actual_frames, ideal_duration_ms, refresh_rate);
  UMA_HISTOGRAM_PERCENTAGE(kFolderShowHideAnimationSmoothness, smoothness);
}

void RecordPaginationAnimationSmoothness(int actual_frames,
                                         int ideal_duration_ms,
                                         float refresh_rate) {
  const int smoothness = CalculateAnimationSmoothness(
      actual_frames, ideal_duration_ms, refresh_rate);
  UMA_HISTOGRAM_PERCENTAGE(kPaginationTransitionAnimationSmoothness,
                           smoothness);
}

APP_LIST_EXPORT void RecordSearchResultOpenSource(
    const SearchResult* result,
    const AppListModel* model,
    const SearchModel* search_model) {
  // Record the search metric if the SearchResult is not a suggested app.
  if (result->display_type() == ash::SearchResultDisplayType::kRecommendation)
    return;

  ApplistSearchResultOpenedSource source;
  AppListViewState state = model->state_fullscreen();
  if (search_model->tablet_mode()) {
    source = ApplistSearchResultOpenedSource::kFullscreenTablet;
  } else {
    source = state == AppListViewState::HALF
                 ? ApplistSearchResultOpenedSource::kHalfClamshell
                 : ApplistSearchResultOpenedSource::kFullscreenClamshell;
  }
  UMA_HISTOGRAM_ENUMERATION(
      kAppListSearchResultOpenSourceHistogram, source,
      ApplistSearchResultOpenedSource::kMaxApplistSearchResultOpenedSource);
}

}  // namespace app_list

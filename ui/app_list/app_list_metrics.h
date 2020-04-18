// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_APP_LIST_METRICS_H_
#define UI_APP_LIST_APP_LIST_METRICS_H_

#include "ui/app_list/app_list_export.h"

namespace app_list {

class AppListModel;
class SearchModel;
class SearchResult;

// The UMA histogram that logs the input latency from input event to the
// representation time of the shown launcher UI.
constexpr char kAppListShowInputLatencyHistogram[] =
    "Apps.AppListShow.InputLatency";

// The UMA histogram that logs the input latency from input event to the
// representation time of the dismissed launcher UI.
constexpr char kAppListHideInputLatencyHistogram[] =
    "Apps.AppListHide.InputLatency";

void RecordFolderShowHideAnimationSmoothness(int actual_frames,
                                             int ideal_duration_ms,
                                             float refresh_rate);

void RecordPaginationAnimationSmoothness(int actual_frames,
                                         int ideal_duration_ms,
                                         float refresh_rate);

APP_LIST_EXPORT void RecordSearchResultOpenSource(
    const SearchResult* result,
    const AppListModel* model,
    const SearchModel* search_model);

}  // namespace app_list

#endif  // UI_APP_LIST_APP_LIST_METRICS_H_

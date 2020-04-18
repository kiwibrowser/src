// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/search_controller.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/app_list/app_list_model_updater.h"
#include "chrome/browser/ui/app_list/search/chrome_search_result.h"
#include "chrome/browser/ui/app_list/search/search_provider.h"

namespace app_list {

SearchController::SearchController(AppListModelUpdater* model_updater)
    : mixer_(std::make_unique<Mixer>(model_updater)) {}

SearchController::~SearchController() {}

void SearchController::Start(const base::string16& raw_query) {
  last_raw_query_ = raw_query;

  base::string16 query;
  base::TrimWhitespace(raw_query, base::TRIM_ALL, &query);

  dispatching_query_ = true;
  for (const auto& provider : providers_)
    provider->Start(query);

  dispatching_query_ = false;
  query_for_recommendation_ = query.empty();

  OnResultsChanged();
}

void SearchController::OpenResult(ChromeSearchResult* result, int event_flags) {
  // This can happen in certain circumstances due to races. See
  // https://crbug.com/534772
  if (!result)
    return;

  result->Open(event_flags);
}

void SearchController::InvokeResultAction(ChromeSearchResult* result,
                                          int action_index,
                                          int event_flags) {
  // TODO(xiyuan): Hook up with user learning.
  result->InvokeAction(action_index, event_flags);
}

size_t SearchController::AddGroup(size_t max_results,
                                  double multiplier,
                                  double boost) {
  return mixer_->AddGroup(max_results, multiplier, boost);
}

void SearchController::AddProvider(size_t group_id,
                                   std::unique_ptr<SearchProvider> provider) {
  provider->set_result_changed_callback(
      base::Bind(&SearchController::OnResultsChanged, base::Unretained(this)));
  mixer_->AddProviderToGroup(group_id, provider.get());
  providers_.emplace_back(std::move(provider));
}

void SearchController::OnResultsChanged() {
  if (dispatching_query_)
    return;

  size_t num_max_results =
      query_for_recommendation_ ? kNumStartPageTiles : kMaxSearchResults;
  mixer_->MixAndPublish(num_max_results);
}

ChromeSearchResult* SearchController::FindSearchResult(
    const std::string& result_id) {
  for (const auto& provider : providers_) {
    for (const auto& result : provider->results()) {
      if (result->id() == result_id)
        return result.get();
    }
  }
  return nullptr;
}

ChromeSearchResult* SearchController::GetResultByTitleForTest(
    const std::string& title) {
  base::string16 target_title = base::ASCIIToUTF16(title);
  for (const auto& provider : providers_) {
    for (const auto& result : provider->results()) {
      if (result->title() == target_title &&
          result->result_type() == ash::SearchResultType::kInstalledApp &&
          result->display_type() !=
              ash::SearchResultDisplayType::kRecommendation) {
        return result.get();
      }
    }
  }
  return nullptr;
}

}  // namespace app_list

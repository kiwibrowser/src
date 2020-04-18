// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_SEARCH_PROVIDER_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_SEARCH_PROVIDER_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/strings/string16.h"

class ChromeSearchResult;

namespace app_list {

class SearchProvider {
 public:
  using Results = std::vector<std::unique_ptr<ChromeSearchResult>>;
  using ResultChangedCallback = base::Closure;

  SearchProvider();
  virtual ~SearchProvider();

  // Invoked to start a query.
  virtual void Start(const base::string16& query) = 0;

  void set_result_changed_callback(const ResultChangedCallback& callback) {
    result_changed_callback_ = callback;
  }

  const Results& results() const { return results_; }

 protected:
  // Interface for the derived class to generate search results.
  void Add(std::unique_ptr<ChromeSearchResult> result);

  // Swaps the internal results with |new_results|.
  // This is useful when multiple results will be added, and the notification is
  // desired to be done only once when all results are added.
  void SwapResults(Results* new_results);

  void ClearResults();

 private:
  void FireResultChanged();

  ResultChangedCallback result_changed_callback_;
  Results results_;

  DISALLOW_COPY_AND_ASSIGN(SearchProvider);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_SEARCH_PROVIDER_H_

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_APP_SEARCH_PROVIDER_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_APP_SEARCH_PROVIDER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "chrome/browser/ui/app_list/search/search_provider.h"

class AppListControllerDelegate;
class AppListModelUpdater;
class Profile;

namespace base {
class Clock;
}

namespace app_list {

class AppSearchProvider : public SearchProvider {
 public:
  class App;
  class DataSource;
  using Apps = std::vector<std::unique_ptr<App>>;

  // |clock| should be used by tests that needs to overrides the time.
  // Otherwise, pass a base::DefaultClock instance. This doesn't take the
  // ownership of the clock. |clock| must outlive the AppSearchProvider
  // instance.
  AppSearchProvider(Profile* profile,
                    AppListControllerDelegate* list_controller,
                    base::Clock* clock,
                    AppListModelUpdater* model_updater);
  ~AppSearchProvider() override;

  // SearchProvider overrides:
  void Start(const base::string16& query) override;

  // Refresh indexed app data and update search results. When |force_inline| is
  // set to true, search results is updated before returning from the function.
  // Otherwise, search results would be grouped, i.e. multiple calls would only
  // update search results once.
  void RefreshAppsAndUpdateResults(bool force_inline);

 private:
  void RefreshApps();
  void UpdateResults();
  void UpdateRecommendedResults(
      const base::flat_map<std::string, uint16_t>& id_to_app_list_index);
  void UpdateQueriedResults();

  AppListControllerDelegate* const list_controller_;
  base::string16 query_;
  Apps apps_;
  AppListModelUpdater* const model_updater_;
  base::Clock* clock_;
  std::vector<std::unique_ptr<DataSource>> data_sources_;
  base::WeakPtrFactory<AppSearchProvider> update_results_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppSearchProvider);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_APP_SEARCH_PROVIDER_H_

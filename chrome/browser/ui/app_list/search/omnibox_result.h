// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_OMNIBOX_RESULT_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_OMNIBOX_RESULT_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/app_list/search/chrome_search_result.h"
#include "components/omnibox/browser/autocomplete_match.h"

class AppListControllerDelegate;
class AutocompleteController;
class Profile;

namespace app_list {

class OmniboxResult : public ChromeSearchResult {
 public:
  OmniboxResult(Profile* profile,
                AppListControllerDelegate* list_controller,
                AutocompleteController* autocomplete_controller,
                const AutocompleteMatch& match);
  ~OmniboxResult() override;

  // ChromeSearchResult overrides:
  void Open(int event_flags) override;

 private:
  void UpdateIcon();

  void UpdateTitleAndDetails();

  // Returns true if |match_| indicates a url search result with non-empty
  // description.
  bool IsUrlResultWithDescription() const;

  Profile* profile_;
  AppListControllerDelegate* list_controller_;
  AutocompleteController* autocomplete_controller_;
  AutocompleteMatch match_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxResult);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_OMNIBOX_RESULT_H_

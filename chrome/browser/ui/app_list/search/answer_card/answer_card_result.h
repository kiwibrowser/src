// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_RESULT_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_RESULT_H_

#include <memory>
#include <string>

#include "chrome/browser/ui/app_list/search/chrome_search_result.h"

class AppListControllerDelegate;
class Profile;

namespace app_list {

class AnswerCardContents;

// Result of AnswerCardSearchProvider.
class AnswerCardResult : public ChromeSearchResult {
 public:
  AnswerCardResult(Profile* profile,
                   AppListControllerDelegate* list_controller,
                   const std::string& result_url,
                   const std::string& stripped_result_url,
                   const base::string16& result_title,
                   AnswerCardContents* contents);

  ~AnswerCardResult() override;

  void OnContentsDestroying();

  void Open(int event_flags) override;

 private:
  Profile* const profile_;                            // Unowned
  AppListControllerDelegate* const list_controller_;  // Unowned
  AnswerCardContents* contents_;                      // Unowned

  DISALLOW_COPY_AND_ASSIGN(AnswerCardResult);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_RESULT_H_

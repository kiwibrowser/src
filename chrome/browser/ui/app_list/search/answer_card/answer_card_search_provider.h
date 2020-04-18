// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_SEARCH_PROVIDER_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_SEARCH_PROVIDER_H_

#include <memory>
#include <string>

#include "base/time/time.h"
#include "chrome/browser/ui/app_list/search/answer_card/answer_card_contents.h"
#include "chrome/browser/ui/app_list/search/search_provider.h"
#include "url/gurl.h"

class AppListControllerDelegate;
class AppListModelUpdater;
class Profile;
class TemplateURLService;

namespace app_list {

// Search provider for the answer card.
class AnswerCardSearchProvider : public SearchProvider,
                                 public AnswerCardContents::Delegate {
 public:
  AnswerCardSearchProvider(Profile* profile,
                           AppListModelUpdater* model_updater,
                           AppListControllerDelegate* list_controller,
                           std::unique_ptr<AnswerCardContents> contents0,
                           std::unique_ptr<AnswerCardContents> contents1);

  ~AnswerCardSearchProvider() override;

  // SearchProvider overrides:
  void Start(const base::string16& query) override;

  // AnswerCardContents::Delegate overrides:
  void UpdatePreferredSize(const AnswerCardContents* source) override;
  void DidFinishNavigation(const AnswerCardContents* source,
                           const GURL& url,
                           bool has_error,
                           bool has_answer_card,
                           const std::string& result_title,
                           const std::string& issued_query) override;
  void OnContentsReady(const AnswerCardContents* source) override;

 private:
  enum class RequestState {
    NO_RESULT,
    HAVE_RESULT_LOADING,
    HAVE_RESULT_LOADED
  };

  // State of navigation for a single AnswerCardContents. There are 2 instances
  // of AnswerCardContents: one is used to show the current answer, and the
  // other for loading an answer for the next query. Once the answer has been
  // loaded, the roles of contents instances get swapped.
  struct NavigationContext {
    NavigationContext();
    ~NavigationContext();

    void StartServerRequest(const GURL& url);
    void Clear();

    // The source of answer card contents.
    std::unique_ptr<AnswerCardContents> contents;

    // State of a server request.
    RequestState state = RequestState::NO_RESULT;

    // Url to open when the user clicks at the result.
    std::string result_url;

    // Title of the result.
    std::string result_title;

    DISALLOW_COPY_AND_ASSIGN(NavigationContext);
  };

  void UpdateResult();
  // Returns Url to open when the user clicks at the result for |query|.
  std::string GetResultUrl(const base::string16& query) const;
  void DeleteCurrentResult();
  NavigationContext& GetCurrentNavigationContext();
  NavigationContext& GetNavigationContextForLoading();

  // Unowned pointer to the associated profile.
  Profile* const profile_;

  // Unowned pointer to app list model updater.
  AppListModelUpdater* const model_updater_;

  // Unowned pointer to app list controller.
  AppListControllerDelegate* const list_controller_;

  // Index of the navigation contents corresponding to the current result. 1 -
  // |current_navigation_context_| will be used for loading the next card, or is
  // already used loading a new card. This pointer switches to the other
  // contents after the card gets successfully loaded.
  int current_navigation_context_ = 0;

  // States of card navigation. one is used to show the current answer, and
  // another for loading an answer for the next query.
  NavigationContext navigation_contexts_[2];

  // If valid, URL of the answer server. Otherwise, search answers are disabled.
  GURL answer_server_url_;

  // URL of the current answer server request.
  GURL current_request_url_;

  // Time when the current server request started.
  base::TimeTicks server_request_start_time_;

  // Time when the current server response loaded.
  base::TimeTicks answer_loaded_time_;

  // Unowned pointer to template URL service.
  TemplateURLService* const template_url_service_;

  DISALLOW_COPY_AND_ASSIGN(AnswerCardSearchProvider);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_ANSWER_CARD_ANSWER_CARD_SEARCH_PROVIDER_H_

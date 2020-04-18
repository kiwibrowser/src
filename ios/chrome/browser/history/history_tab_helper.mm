// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/history/history_tab_helper.h"

#include "base/memory/ptr_util.h"
#include "components/history/core/browser/history_constants.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/web_state/navigation_context.h"
#import "ios/web/public/web_state/web_state.h"
#include "net/http/http_response_headers.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(HistoryTabHelper);

HistoryTabHelper::~HistoryTabHelper() {
  DCHECK(!web_state_);
}

void HistoryTabHelper::UpdateHistoryPageTitle(const web::NavigationItem& item) {
  DCHECK(!delay_notification_);

  const base::string16& title = item.GetTitleForDisplay();
  // Don't update the history if current entry has no title.
  if (title.empty() ||
      title == l10n_util::GetStringUTF16(IDS_DEFAULT_TAB_TITLE)) {
    return;
  }

  history::HistoryService* history_service = GetHistoryService();
  if (history_service) {
    history_service->SetPageTitle(item.GetVirtualURL(), title);
  }
}

void HistoryTabHelper::SetDelayHistoryServiceNotification(
    bool delay_notification) {
  delay_notification_ = delay_notification;
  if (delay_notification_) {
    return;
  }

  history::HistoryService* history_service = GetHistoryService();
  if (history_service) {
    for (const auto& add_page_args : recorded_navigations_) {
      history_service->AddPage(add_page_args);
    }
  }

  std::vector<history::HistoryAddPageArgs> empty_vector;
  std::swap(recorded_navigations_, empty_vector);

  web::NavigationItem* last_committed_item =
      web_state_->GetNavigationManager()->GetLastCommittedItem();
  if (last_committed_item) {
    UpdateHistoryPageTitle(*last_committed_item);
  }
}

HistoryTabHelper::HistoryTabHelper(web::WebState* web_state)
    : web_state_(web_state) {
  web_state_->AddObserver(this);
}

void HistoryTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  DCHECK_EQ(web_state_, web_state);
  if (web_state_->GetBrowserState()->IsOffTheRecord()) {
    return;
  }

  // Do not record failed navigation nor 404 to the history (to prevent them
  // from showing up as Most Visited tiles on NTP).
  if (navigation_context->GetError()) {
    return;
  }

  if (navigation_context->GetResponseHeaders() &&
      navigation_context->GetResponseHeaders()->response_code() == 404) {
    return;
  }

  if (navigation_context->IsDownload()) {
    return;
  }

  if (!navigation_context->HasCommitted() &&
      !navigation_context->IsSameDocument()) {
    // Navigation was replaced.
    return;
  }

  DCHECK(web_state->GetNavigationManager()->GetVisibleItem());
  web::NavigationItem* visible_item =
      web_state_->GetNavigationManager()->GetVisibleItem();
  DCHECK(!visible_item->GetTimestamp().is_null());

  // Do not update the history database for back/forward navigations.
  // TODO(crbug.com/661667): on iOS the navigation is not currently tagged with
  // a ui::PAGE_TRANSITION_FORWARD_BACK transition.
  const ui::PageTransition transition = visible_item->GetTransitionType();
  if (transition & ui::PAGE_TRANSITION_FORWARD_BACK) {
    return;
  }

  // Do not update the history database for data: urls. This diverges from
  // desktop, but prevents dumping huge view-source urls into the history
  // database. Keep it NDEBUG only because view-source:// URLs are enabled
  // on NDEBUG builds only.
  const GURL& url = visible_item->GetURL();
#ifndef NDEBUG
  if (url.SchemeIs(url::kDataScheme)) {
    return;
  }
#endif

  num_title_changes_ = 0;

  history::RedirectList redirects;
  const GURL& original_url = visible_item->GetOriginalRequestURL();
  const GURL& referrer_url = visible_item->GetReferrer().url;
  if (original_url != url) {
    // Simulate a valid redirect chain in case of URLs that have been modified
    // by CRWWebController -finishHistoryNavigationFromEntry:.
    if (transition & ui::PAGE_TRANSITION_CLIENT_REDIRECT ||
        url.EqualsIgnoringRef(original_url)) {
      redirects.push_back(referrer_url);
    }
    // TODO(crbug.com/703872): the redirect chain is not constructed the same
    // way as desktop so this part needs to be revised.
    redirects.push_back(original_url);
    redirects.push_back(url);
  }

  // Navigations originating from New Tab Page or Reading List should not
  // contribute to Most Visited.
  const bool consider_for_ntp_most_visited =
      referrer_url != kNewTabPageReferrerURL &&
      referrer_url != kReadingListReferrerURL;

  // Top-level frame navigations are visible; everything else is hidden.
  // Also hide top-level navigations that result in an error in order to
  // prevent the omnibox from suggesting URLs that have never been navigated
  // to successfully.  (If a top-level navigation to the URL succeeds at some
  // point, the URL will be unhidden and thus eligible to be suggested by the
  // omnibox.)
  const bool hidden =
      navigation_context->GetError() ||
      (navigation_context->GetResponseHeaders() &&
       navigation_context->GetResponseHeaders()->response_code() >= 400 &&
       navigation_context->GetResponseHeaders()->response_code() > 600) ||
      !ui::PageTransitionIsMainFrame(navigation_context->GetPageTransition());
  history::HistoryAddPageArgs add_page_args(
      url, visible_item->GetTimestamp(), this, visible_item->GetUniqueID(),
      referrer_url, redirects, transition, hidden, history::SOURCE_BROWSED,
      /*did_replace_entry=*/false, consider_for_ntp_most_visited);

  if (delay_notification_) {
    recorded_navigations_.push_back(std::move(add_page_args));
  } else {
    DCHECK(recorded_navigations_.empty());

    history::HistoryService* history_service = GetHistoryService();
    if (history_service) {
      history_service->AddPage(add_page_args);
      UpdateHistoryPageTitle(*visible_item);
    }
  }
}

void HistoryTabHelper::PageLoaded(
    web::WebState* web_state,
    web::PageLoadCompletionStatus load_completion_status) {
  last_load_completion_ = base::TimeTicks::Now();
}

void HistoryTabHelper::TitleWasSet(web::WebState* web_state) {
  DCHECK_EQ(web_state_, web_state);
  if (delay_notification_) {
    return;
  }

  // Protect against pages changing their title too often during page load.
  if (num_title_changes_ >= history::kMaxTitleChanges)
    return;

  // Only store page titles into history if they were set while the page was
  // loading or during a brief span after load is complete. This fixes the case
  // where a page uses a title change to alert a user of a situation but that
  // title change ends up saved in history.
  if (web_state->IsLoading() ||
      (base::TimeTicks::Now() - last_load_completion_ <
       history::GetTitleSettingWindow())) {
    web::NavigationItem* last_committed_item =
        web_state_->GetNavigationManager()->GetLastCommittedItem();
    if (last_committed_item) {
      UpdateHistoryPageTitle(*last_committed_item);
      ++num_title_changes_;
    }
  }
}

void HistoryTabHelper::WebStateDestroyed(web::WebState* web_state) {
  DCHECK_EQ(web_state_, web_state);
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

history::HistoryService* HistoryTabHelper::GetHistoryService() {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(web_state_->GetBrowserState());
  if (browser_state->IsOffTheRecord())
    return nullptr;

  return ios::HistoryServiceFactory::GetForBrowserState(
      browser_state, ServiceAccessType::IMPLICIT_ACCESS);
}

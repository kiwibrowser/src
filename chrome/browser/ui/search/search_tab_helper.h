// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SEARCH_SEARCH_TAB_HELPER_H_
#define CHROME_BROWSER_UI_SEARCH_SEARCH_TAB_HELPER_H_

#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/search/instant_service_observer.h"
#include "chrome/browser/ui/search/search_ipc_router.h"
#include "chrome/common/search/instant_types.h"
#include "chrome/common/search/ntp_logging_events.h"
#include "components/ntp_tiles/ntp_tile_impression.h"
#include "components/omnibox/common/omnibox_focus_state.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

#if defined(OS_ANDROID)
#error "Instant is only used on desktop";
#endif

namespace content {
class WebContents;
struct LoadCommittedDetails;
}

class GURL;
class InstantService;
class OmniboxView;
class Profile;
class SearchIPCRouterTest;

// This is the browser-side, per-tab implementation of the embeddedSearch API
// (see https://www.chromium.org/embeddedsearch).
class SearchTabHelper : public content::WebContentsObserver,
                        public content::WebContentsUserData<SearchTabHelper>,
                        public InstantServiceObserver,
                        public SearchIPCRouter::Delegate {
 public:
  ~SearchTabHelper() override;

  // Invoked when the omnibox input state is changed in some way that might
  // affect the search mode.
  void OmniboxInputStateChanged();

  // Called to indicate that the omnibox focus state changed with the given
  // |reason|.
  void OmniboxFocusChanged(OmniboxFocusState state,
                           OmniboxFocusChangeReason reason);

  // Called when the tab corresponding to |this| instance is activated.
  void OnTabActivated();

  // Called when the tab corresponding to |this| instance is deactivated.
  void OnTabDeactivated();

  SearchIPCRouter& ipc_router_for_testing() { return ipc_router_; }

 private:
  friend class content::WebContentsUserData<SearchTabHelper>;
  friend class SearchIPCRouterTest;

  FRIEND_TEST_ALL_PREFIXES(SearchTabHelperTest, ChromeIdentityCheckMatch);
  FRIEND_TEST_ALL_PREFIXES(SearchTabHelperTest,
                           ChromeIdentityCheckMatchSlightlyDifferentGmail);
  FRIEND_TEST_ALL_PREFIXES(SearchTabHelperTest,
                           ChromeIdentityCheckMatchSlightlyDifferentGmail2);
  FRIEND_TEST_ALL_PREFIXES(SearchTabHelperTest, ChromeIdentityCheckMismatch);
  FRIEND_TEST_ALL_PREFIXES(SearchTabHelperTest,
                           ChromeIdentityCheckSignedOutMismatch);
  FRIEND_TEST_ALL_PREFIXES(SearchTabHelperTest, HistorySyncCheckSyncing);
  FRIEND_TEST_ALL_PREFIXES(SearchTabHelperTest, HistorySyncCheckNotSyncing);

  explicit SearchTabHelper(content::WebContents* web_contents);

  // Overridden from contents::WebContentsObserver:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void TitleWasSet(content::NavigationEntry* entry) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;

  // Overridden from SearchIPCRouter::Delegate:
  void FocusOmnibox(OmniboxFocusState state) override;
  void OnDeleteMostVisitedItem(const GURL& url) override;
  void OnUndoMostVisitedDeletion(const GURL& url) override;
  void OnUndoAllMostVisitedDeletions() override;
  void OnLogEvent(NTPLoggingEventType event, base::TimeDelta time) override;
  void OnLogMostVisitedImpression(
      const ntp_tiles::NTPTileImpression& impression) override;
  void OnLogMostVisitedNavigation(
      const ntp_tiles::NTPTileImpression& impression) override;
  void PasteIntoOmnibox(const base::string16& text) override;
  bool ChromeIdentityCheck(const base::string16& identity) override;
  bool HistorySyncCheck() override;
  void OnSetCustomBackgroundURL(const GURL& url) override;

  // Overridden from InstantServiceObserver:
  void ThemeInfoChanged(const ThemeBackgroundInfo& theme_info) override;
  void MostVisitedItemsChanged(
      const std::vector<InstantMostVisitedItem>& items) override;

  OmniboxView* GetOmniboxView();
  const OmniboxView* GetOmniboxView() const;

  Profile* profile() const;

  // Returns whether input is in progress, i.e. if the omnibox has focus and the
  // active tab is in mode SEARCH_SUGGESTIONS.
  bool IsInputInProgress() const;

  content::WebContents* web_contents_;

  SearchIPCRouter ipc_router_;

  InstantService* instant_service_;

  bool is_setting_title_ = false;

  DISALLOW_COPY_AND_ASSIGN(SearchTabHelper);
};

#endif  // CHROME_BROWSER_UI_SEARCH_SEARCH_TAB_HELPER_H_

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_APPS_INTENT_HELPER_APPS_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_CHROMEOS_APPS_INTENT_HELPER_APPS_NAVIGATION_THROTTLE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/apps/intent_helper/apps_navigation_types.h"
#include "content/public/browser/navigation_throttle.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace chromeos {

// Allows navigation to be routed to an installed app on Chrome OS, and provides
// a static method for showing an intent picker for the current URL to display
// any handling apps.
class AppsNavigationThrottle : public content::NavigationThrottle {
 public:
  // Restricts the amount of apps displayed to the user without the need of a
  // ScrollView.
  enum { kMaxAppResults = 3 };

  // Possibly creates a navigation throttle that checks if any installed apps
  // can handle the URL being navigated to. The user is prompted if they wish to
  // open the app or remain in the browser.
  static std::unique_ptr<content::NavigationThrottle> MaybeCreate(
      content::NavigationHandle* handle);

  // Queries for installed apps which can handle |url|, and displays the intent
  // picker bubble for |web_contents|.
  static void ShowIntentPickerBubble(content::WebContents* web_contents,
                                     const GURL& url);

  // Called when the intent picker is closed for |url|, in |web_contents|, with
  // |launch_name| as the (possibly empty) action to be triggered based on
  // |app_type|. |close_reason| gives the reason for the picker being closed,
  // and |should_persist| is true if the user indicated they wish to remember
  // the choice made.
  static void OnIntentPickerClosed(content::WebContents* web_contents,
                                   const GURL& url,
                                   const std::string& launch_name,
                                   AppType app_type,
                                   IntentPickerCloseReason close_reason,
                                   bool should_persist);

  static void RecordUma(const std::string& selected_app_package,
                        AppType app_type,
                        IntentPickerCloseReason close_reason,
                        bool should_persist);

  static bool ShouldOverrideUrlLoadingForTesting(const GURL& previous_url,
                                                 const GURL& current_url);

  AppsNavigationThrottle(content::NavigationHandle* navigation_handle,
                         bool arc_enabled);
  ~AppsNavigationThrottle() override;

  // content::NavigationHandle overrides
  const char* GetNameForLogging() override;
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;

 private:
  FRIEND_TEST_ALL_PREFIXES(AppsNavigationThrottleTest, TestGetPickerAction);
  FRIEND_TEST_ALL_PREFIXES(AppsNavigationThrottleTest,
                           TestGetDestinationPlatform);

  // These enums are used to define the buckets for an enumerated UMA histogram
  // and need to be synced with histograms.xml. This enum class should also be
  // treated as append-only.
  enum class PickerAction : int {
    ERROR = 0,
    // DIALOG_DEACTIVATED keeps track of the user dismissing the UI via clicking
    // the close button or clicking outside of the IntentPickerBubbleView
    // surface. As with CHROME_PRESSED, the user stays in Chrome, however we
    // keep both options since CHROME_PRESSED is tied to an explicit intent of
    // staying in Chrome, not only just getting rid of the
    // IntentPickerBubbleView UI.
    DIALOG_DEACTIVATED = 1,
    OBSOLETE_ALWAYS_PRESSED = 2,
    OBSOLETE_JUST_ONCE_PRESSED = 3,
    PREFERRED_ACTIVITY_FOUND = 4,
    // The prefix "CHROME"/"ARC_APP"/"PWA_APP" determines whether the user
    // pressed [Stay in Chrome] or [Use app] at IntentPickerBubbleView.
    // "PREFERRED" denotes when the user decides to save this selection, whether
    // an app or Chrome was selected.
    CHROME_PRESSED = 5,
    CHROME_PREFERRED_PRESSED = 6,
    ARC_APP_PRESSED = 7,
    ARC_APP_PREFERRED_PRESSED = 8,
    PWA_APP_PRESSED = 9,
    SIZE,
    INVALID = SIZE,
  };

  // As for PickerAction, these define the buckets for an UMA histogram, so this
  // must be treated in an append-only fashion. This helps especify where a
  // navigation will continue.
  enum class Platform : int {
    ARC = 0,
    CHROME = 1,
    PWA = 2,
    SIZE,
  };

  // Determines the destination of the current navigation. We know that if the
  // |picker_action| is either ERROR or DIALOG_DEACTIVATED the navigation MUST
  // stay in Chrome, and when |picker_action| is PWA_APP_PRESSED the navigation
  // goes to a PWA. Otherwise we can assume the navigation goes to ARC with the
  // exception of the |selected_launch_name| being Chrome.
  static Platform GetDestinationPlatform(
      const std::string& selected_launch_name,
      PickerAction picker_action);

  // Converts the provided |app_type|, |close_reason| and |should_persist|
  // boolean to a PickerAction value for recording in UMA.
  static PickerAction GetPickerAction(AppType app_type,
                                      IntentPickerCloseReason close_reason,
                                      bool should_persist);

  static void FindPwaForUrlAndShowIntentPickerForApps(
      content::WebContents* web_contents,
      const GURL& url,
      std::vector<IntentPickerAppInfo> apps);

  // If an installed PWA exists that can handle |url|, prepends it to |apps| and
  // returns the new list.
  static std::vector<IntentPickerAppInfo> FindPwaForUrl(
      content::WebContents* web_contents,
      const GURL& url,
      std::vector<IntentPickerAppInfo> apps);

  static void ShowIntentPickerBubbleForApps(
      content::WebContents* web_contents,
      const GURL& url,
      std::vector<IntentPickerAppInfo> apps);

  static void CloseOrGoBack(content::WebContents* web_contents);

  void CancelNavigation();

  content::NavigationThrottle::ThrottleCheckResult HandleRequest();

  // Passed as a callback to allow ARC-specific code to asynchronously inform
  // this object of the apps which can handle this URL, and optionally request
  // that the navigation be completely cancelled (e.g. if a preferred app has
  // been opened).
  void OnDeferredNavigationProcessed(AppsNavigationAction action,
                                     std::vector<IntentPickerAppInfo> apps);

  void CloseTab();

  // A reference to the starting GURL.
  GURL starting_url_;

  // True if ARC is enabled, false otherwise.
  const bool arc_enabled_;

  // Keeps track of whether we already shown the UI or preferred app. Since
  // AppsNavigationThrottle cannot wait for the user (due to the non-blocking
  // nature of the feature) the best we can do is check if we launched a
  // preferred app or asked the UI to be shown, this flag ensures we never
  // trigger the UI twice for the same throttle.
  bool ui_displayed_;

  base::WeakPtrFactory<AppsNavigationThrottle> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppsNavigationThrottle);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_APPS_INTENT_HELPER_APPS_NAVIGATION_THROTTLE_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/bookmark_app_experimental_navigation_throttle.h"

#include <memory>

#include "base/bind.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/extensions/bookmark_app_navigation_throttle_utils.h"
#include "chrome/browser/prerender/prerender_contents.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/extension.h"
#include "ui/base/mojo/window_open_disposition_struct_traits.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

using content::BrowserThread;

namespace extensions {

namespace {

// Non-app site navigations: The majority of navigations will be in-browser to
// sites for which there is no app installed. These navigations offer no insight
// so we avoid recording their outcome.

void RecordProceedWithTransitionType(ui::PageTransition transition_type) {
  if (PageTransitionCoreTypeIs(transition_type, ui::PAGE_TRANSITION_LINK)) {
    // Link navigations are a special case and shouldn't use this code path.
    NOTREACHED();
  } else if (PageTransitionCoreTypeIs(transition_type,
                                      ui::PAGE_TRANSITION_TYPED)) {
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kProceedTransitionTyped);
  } else if (PageTransitionCoreTypeIs(transition_type,
                                      ui::PAGE_TRANSITION_AUTO_BOOKMARK)) {
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kProceedTransitionAutoBookmark);
  } else if (PageTransitionCoreTypeIs(transition_type,
                                      ui::PAGE_TRANSITION_AUTO_SUBFRAME)) {
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kProceedTransitionAutoSubframe);
  } else if (PageTransitionCoreTypeIs(transition_type,
                                      ui::PAGE_TRANSITION_MANUAL_SUBFRAME)) {
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kProceedTransitionManualSubframe);
  } else if (PageTransitionCoreTypeIs(transition_type,
                                      ui::PAGE_TRANSITION_GENERATED)) {
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kProceedTransitionGenerated);
  } else if (PageTransitionCoreTypeIs(transition_type,
                                      ui::PAGE_TRANSITION_AUTO_TOPLEVEL)) {
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kProceedTransitionAutoToplevel);
  } else if (PageTransitionCoreTypeIs(transition_type,
                                      ui::PAGE_TRANSITION_FORM_SUBMIT)) {
    // Form navigations are a special case and shouldn't use this code path.
    // TODO(crbug.com/772803): Add NOTREACHED() once form navigations are
    // handled.
  } else if (PageTransitionCoreTypeIs(transition_type,
                                      ui::PAGE_TRANSITION_RELOAD)) {
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kProceedTransitionReload);
  } else if (PageTransitionCoreTypeIs(transition_type,
                                      ui::PAGE_TRANSITION_KEYWORD)) {
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kProceedTransitionKeyword);
  } else if (PageTransitionCoreTypeIs(transition_type,
                                      ui::PAGE_TRANSITION_KEYWORD_GENERATED)) {
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::
            kProceedTransitionKeywordGenerated);
  } else {
    NOTREACHED();
  }
}

void RecordProceedWithDisposition(WindowOpenDisposition disposition) {
  BookmarkAppNavigationThrottleResult result =
      BookmarkAppNavigationThrottleResult::kProceedDispositionSingletonTab;
  switch (disposition) {
    case WindowOpenDisposition::UNKNOWN:
    case WindowOpenDisposition::SAVE_TO_DISK:
    case WindowOpenDisposition::IGNORE_ACTION:
      // These values don't result in a navigation, so they will never be
      // passed to this class.
      NOTREACHED();
      break;
    case WindowOpenDisposition::CURRENT_TAB:
    case WindowOpenDisposition::NEW_FOREGROUND_TAB:
    case WindowOpenDisposition::NEW_WINDOW:
    case WindowOpenDisposition::OFF_THE_RECORD:
      // These navigations are special cases and are handled elsewhere.
      NOTREACHED();
      break;
    case WindowOpenDisposition::SINGLETON_TAB:
      result =
          BookmarkAppNavigationThrottleResult::kProceedDispositionSingletonTab;
      break;
    case WindowOpenDisposition::NEW_BACKGROUND_TAB:
      result = BookmarkAppNavigationThrottleResult::
          kProceedDispositionNewBackgroundTab;
      break;
    case WindowOpenDisposition::NEW_POPUP:
      result = BookmarkAppNavigationThrottleResult::kProceedDispositionNewPopup;
      break;
    case WindowOpenDisposition::SWITCH_TO_TAB:
      result =
          BookmarkAppNavigationThrottleResult::kProceedDispositionSwitchToTab;
      break;
  }

  RecordBookmarkAppNavigationThrottleResult(result);
}

}  // namespace

// static
std::unique_ptr<content::NavigationThrottle>
BookmarkAppExperimentalNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  DVLOG(1) << "Considering URL for interception: "
           << navigation_handle->GetURL().spec();
  if (!navigation_handle->IsInMainFrame()) {
    DVLOG(1) << "Don't intercept: Navigation is not in main frame.";
    return nullptr;
  }

  content::BrowserContext* browser_context =
      navigation_handle->GetWebContents()->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (profile->GetProfileType() == Profile::INCOGNITO_PROFILE) {
    DVLOG(1) << "Don't intercept: Navigation is in incognito.";
    return nullptr;
  }

  DVLOG(1) << "Attaching Bookmark App Navigation Throttle.";
  return std::make_unique<
      extensions::BookmarkAppExperimentalNavigationThrottle>(navigation_handle);
}

BookmarkAppExperimentalNavigationThrottle::
    BookmarkAppExperimentalNavigationThrottle(
        content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle), weak_ptr_factory_(this) {}

BookmarkAppExperimentalNavigationThrottle::
    ~BookmarkAppExperimentalNavigationThrottle() {}

const char* BookmarkAppExperimentalNavigationThrottle::GetNameForLogging() {
  return "BookmarkAppExperimentalNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
BookmarkAppExperimentalNavigationThrottle::WillStartRequest() {
  return ProcessNavigation(false /* is_redirect */);
}

content::NavigationThrottle::ThrottleCheckResult
BookmarkAppExperimentalNavigationThrottle::WillRedirectRequest() {
  return ProcessNavigation(true /* is_redirect */);
}

content::NavigationThrottle::ThrottleCheckResult
BookmarkAppExperimentalNavigationThrottle::ProcessNavigation(bool is_redirect) {
  content::WebContents* source = navigation_handle()->GetWebContents();
  scoped_refptr<const Extension> target_app =
      GetTargetApp(source, navigation_handle()->GetURL());

  if (navigation_handle()->WasStartedFromContextMenu()) {
    DVLOG(1) << "Don't intercept: Navigation started from the context menu.";

    // See "Non-app site navigations" note above.
    if (target_app) {
      RecordBookmarkAppNavigationThrottleResult(
          BookmarkAppNavigationThrottleResult::kProceedStartedFromContextMenu);
    }

    return content::NavigationThrottle::PROCEED;
  }

  ui::PageTransition transition_type = navigation_handle()->GetPageTransition();

  // When launching an app, if the page redirects to an out-of-scope URL, then
  // continue the navigation in a regular browser window. (Launching an app
  // results in an AUTO_BOOKMARK transition).
  //
  // Note that for non-redirecting app launches, GetAppForWindow() might return
  // null, because the navigation's WebContents might not be attached to a
  // window yet.
  //
  // TODO(crbug.com/789051): Possibly fall through to the logic below to
  // open target in app window, if it belongs to an app.
  if (is_redirect && PageTransitionCoreTypeIs(
                         transition_type, ui::PAGE_TRANSITION_AUTO_BOOKMARK)) {
    auto app_for_window = GetAppForWindow(source);
    // If GetAppForWindow returned nullptr, we are already in the browser, so
    // don't open a new tab.
    if (app_for_window && app_for_window != target_app) {
      DVLOG(1) << "Out-of-scope navigation during launch. Opening in Chrome.";
      RecordBookmarkAppNavigationThrottleResult(
          BookmarkAppNavigationThrottleResult::
              kOpenInChromeProceedOutOfScopeLaunch);
      Browser* browser = chrome::FindBrowserWithWebContents(source);
      DCHECK(browser);
      chrome::OpenInChrome(browser);
      return content::NavigationThrottle::PROCEED;
    }
  }

  if (!PageTransitionCoreTypeIs(transition_type, ui::PAGE_TRANSITION_LINK) &&
      !PageTransitionCoreTypeIs(transition_type,
                                ui::PAGE_TRANSITION_FORM_SUBMIT)) {
    DVLOG(1) << "Don't intercept: Transition type is "
             << PageTransitionGetCoreTransitionString(transition_type);
    // We are in one of three possible states:
    //   1. In-browser no-target-app navigations,
    //   2. In-browser same-scope navigations, or
    //   3. In-app same-scope navigations
    // Ignore (1) since that's the majority of navigations and offer no insight.
    if (target_app)
      RecordProceedWithTransitionType(transition_type);

    return content::NavigationThrottle::PROCEED;
  }

  int32_t transition_qualifier = PageTransitionGetQualifier(transition_type);
  if (transition_qualifier & ui::PAGE_TRANSITION_FORWARD_BACK) {
    DVLOG(1) << "Don't intercept: Forward or back navigation.";

    // See "Non-app site navigations" note above.
    if (target_app) {
      RecordBookmarkAppNavigationThrottleResult(
          BookmarkAppNavigationThrottleResult::kProceedTransitionForwardBack);
    }
    return content::NavigationThrottle::PROCEED;
  }

  if (transition_qualifier & ui::PAGE_TRANSITION_FROM_ADDRESS_BAR) {
    DVLOG(1) << "Don't intercept: Address bar navigation.";

    // See "Non-app site navigations" note above.
    if (target_app) {
      RecordBookmarkAppNavigationThrottleResult(
          BookmarkAppNavigationThrottleResult::
              kProceedTransitionFromAddressBar);
    }
    return content::NavigationThrottle::PROCEED;
  }

  const ChromeNavigationUIData* ui_data =
      static_cast<const ChromeNavigationUIData*>(
          navigation_handle()->GetNavigationUIData());

  WindowOpenDisposition disposition = ui_data->window_open_disposition();

  // CURRENT_TAB is used when clicking on links that just navigate the frame
  // We always want to intercept these navigations.
  //
  // FOREGROUND_TAB is used when clicking on links that open a new tab in the
  // foreground e.g. target=_blank links, trying to open a tab inside an app
  // window when there are no regular browser windows, Ctrl + Shift + Clicking
  // a link, etc. We want to ignore Ctrl + Shift + Click navigations.
  // TODO(crbug.com/786835): Stop intercepting FOREGROUND_TAB navigations from
  // Ctrl + Shift + Click.
  //
  // NEW_WINDOW is used when shift + clicking a link or when clicking
  // "Open in new window" in the context menu. We want to intercept these
  // navigations but only if they come from an app.
  // TODO(crbug.com/786838): Stop intercepting NEW_WINDOW navigations outside
  // the app.
  if (disposition != WindowOpenDisposition::CURRENT_TAB &&
      disposition != WindowOpenDisposition::NEW_FOREGROUND_TAB &&
      disposition != WindowOpenDisposition::NEW_WINDOW) {
    DVLOG(1) << "Don't override: Disposition is "
             << mojo::EnumTraits<ui::mojom::WindowOpenDisposition,
                                 WindowOpenDisposition>::ToMojom(disposition);
    RecordProceedWithDisposition(disposition);
    return content::NavigationThrottle::PROCEED;
  }

  scoped_refptr<const Extension> app_for_window = GetAppForWindow(source);

  if (app_for_window == target_app) {
    if (app_for_window) {
      DVLOG(1) << "Don't intercept: The target URL is in the same scope as the "
               << "current app.";

      // We know we are navigating within the same app window (both
      // |app_for_window| and |target_app| are the same and non-null). This is
      // relevant, so record the result.
      RecordBookmarkAppNavigationThrottleResult(
          BookmarkAppNavigationThrottleResult::kProceedInAppSameScope);
    } else {
      DVLOG(1) << "No matching Bookmark App for URL: "
               << navigation_handle()->GetURL();
      // See "Non-app site navigations" note above.
    }
    DVLOG(1) << "Don't intercept: The target URL is in the same scope as the "
             << "current app.";
    return content::NavigationThrottle::PROCEED;
  }

  // If this is a browser tab, and the user is submitting a form, then keep the
  // navigation in the browser tab.
  if (!app_for_window &&
      PageTransitionCoreTypeIs(transition_type,
                               ui::PAGE_TRANSITION_FORM_SUBMIT)) {
    DVLOG(1) << "Keep form submissions in the browser.";
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kProceedInBrowserFormSubmission);
    return content::NavigationThrottle::PROCEED;
  }

  // If this is a browser tab, and the current and target URL are within-scope
  // of the same app, don't intercept the navigation.
  // This ensures that navigating from
  // https://www.youtube.com/ to https://www.youtube.com/some_video doesn't
  // open a new app window if the Youtube app is installed, but navigating from
  // https://www.google.com/ to https://www.google.com/maps does open a new
  // app window if only the Maps app is installed.
  if (!app_for_window && target_app == GetAppForMainFrameURL(source)) {
    DVLOG(1) << "Don't intercept: Keep same-app navigations in the browser.";
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kProceedInBrowserSameScope);
    return content::NavigationThrottle::PROCEED;
  }

  if (target_app) {
    auto* prerender_contents =
        prerender::PrerenderContents::FromWebContents(source);
    if (prerender_contents) {
      // If prerendering, don't launch the app but abort the navigation.
      prerender_contents->Destroy(
          prerender::FINAL_STATUS_NAVIGATION_INTERCEPTED);
      RecordBookmarkAppNavigationThrottleResult(
          BookmarkAppNavigationThrottleResult::kCancelPrerenderContents);
      return content::NavigationThrottle::CANCEL_AND_IGNORE;
    }

    content::NavigationEntry* last_entry =
        source->GetController().GetLastCommittedEntry();
    // We are about to open a new app window context. Record the time since the
    // last navigation in this context. (If it is very small, this context
    // probably redirected immediately, which is a bad user experience.)
    if (last_entry && !last_entry->GetTimestamp().is_null()) {
      UMA_HISTOGRAM_MEDIUM_TIMES(
          "Extensions.BookmarkApp.TimeBetweenOpenAppAndLastNavigation",
          base::Time::Now() - last_entry->GetTimestamp());
    }

    content::NavigationThrottle::ThrottleCheckResult result =
        OpenInAppWindowAndCloseTabIfNecessary(target_app);

    BookmarkAppNavigationThrottleResult open_in_app_result;
    switch (result.action()) {
      case content::NavigationThrottle::DEFER:
        open_in_app_result = BookmarkAppNavigationThrottleResult::
            kDeferMovingContentsToNewAppWindow;
        break;
      case content::NavigationThrottle::CANCEL_AND_IGNORE:
        open_in_app_result =
            BookmarkAppNavigationThrottleResult::kCancelOpenedApp;
        break;
      default:
        NOTREACHED();
        open_in_app_result = BookmarkAppNavigationThrottleResult::
            kDeferMovingContentsToNewAppWindow;
    }

    RecordBookmarkAppNavigationThrottleResult(open_in_app_result);
    return result;
  }

  if (app_for_window) {
    // The experience when navigating to an out-of-scope website inside an app
    // window is not great, so we bounce these navigations back to the browser.
    // TODO(crbug.com/774895): Stop bouncing back to the browser once the
    // experience for out-of-scope navigations improves.
    DVLOG(1) << "Open in new tab.";

    if (source->GetController().IsInitialNavigation()) {
      DVLOG(1) << "In-app initial navigation to out-of-scope URL. "
               << "Opening in popup.";
      RecordBookmarkAppNavigationThrottleResult(
          BookmarkAppNavigationThrottleResult::
              kReparentIntoPopupProceedOutOfScopeInitialNavigation);
      ReparentIntoPopup(source, navigation_handle()->HasUserGesture());
      return content::NavigationThrottle::PROCEED;
    }

    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &BookmarkAppExperimentalNavigationThrottle::OpenInNewTabAndCancel,
            weak_ptr_factory_.GetWeakPtr()));
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kDeferOpenNewTabInAppOutOfScope);
    return content::NavigationThrottle::DEFER;
  }

  DVLOG(1) << "No matching Bookmark App for URL: "
           << navigation_handle()->GetURL();
  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
BookmarkAppExperimentalNavigationThrottle::
    OpenInAppWindowAndCloseTabIfNecessary(
        scoped_refptr<const Extension> target_app) {
  content::WebContents* source = navigation_handle()->GetWebContents();
  if (source->GetController().IsInitialNavigation()) {
    // The first navigation might happen synchronously. This could result in us
    // trying to reparent a WebContents that hasn't been attached to a browser
    // yet. To avoid this we post a task to wait for the WebContents to be
    // attached to a browser window.
    DVLOG(1) << "Defer reparenting WebContents into app window.";
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&BookmarkAppExperimentalNavigationThrottle::
                                      ReparentWebContentsAndResume,
                                  weak_ptr_factory_.GetWeakPtr(), target_app));
    return content::NavigationThrottle::DEFER;
  }

  OpenBookmarkApp(target_app);
  return content::NavigationThrottle::CANCEL_AND_IGNORE;
}

void BookmarkAppExperimentalNavigationThrottle::OpenBookmarkApp(
    scoped_refptr<const Extension> bookmark_app) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  content::WebContents* source = navigation_handle()->GetWebContents();
  content::BrowserContext* browser_context = source->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(browser_context);
  AppLaunchParams launch_params(
      profile, bookmark_app.get(), extensions::LAUNCH_CONTAINER_WINDOW,
      WindowOpenDisposition::CURRENT_TAB, extensions::SOURCE_URL_HANDLER);
  launch_params.override_url = navigation_handle()->GetURL();
  launch_params.opener = source->GetOpener();

  DVLOG(1) << "Opening app.";
  OpenApplication(launch_params);
}

void BookmarkAppExperimentalNavigationThrottle::CloseWebContents() {
  DVLOG(1) << "Closing empty tab.";
  navigation_handle()->GetWebContents()->Close();
}

void BookmarkAppExperimentalNavigationThrottle::OpenInNewTabAndCancel() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  OpenNewForegroundTab(navigation_handle());
  CancelDeferredNavigation(content::NavigationThrottle::CANCEL_AND_IGNORE);
}

void BookmarkAppExperimentalNavigationThrottle::ReparentWebContentsAndResume(
    scoped_refptr<const Extension> target_app) {
  ReparentWebContentsIntoAppBrowser(navigation_handle()->GetWebContents(),
                                    target_app.get());
  Resume();
}

}  // namespace extensions

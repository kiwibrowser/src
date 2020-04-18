// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/bookmark_app_navigation_throttle_utils.h"

#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/launch_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/extensions/api/url_handlers/url_handlers_parser.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "url/gurl.h"

namespace extensions {

namespace {

bool IsWindowedBookmarkApp(const Extension* app,
                           content::BrowserContext* context) {
  if (!app || !app->from_bookmark())
    return false;

  return GetLaunchContainer(extensions::ExtensionPrefs::Get(context), app) ==
         LAUNCH_CONTAINER_WINDOW;
}

}  // namespace

void RecordBookmarkAppNavigationThrottleResult(
    BookmarkAppNavigationThrottleResult result) {
  UMA_HISTOGRAM_ENUMERATION("Extensions.BookmarkApp.NavigationResult", result,
                            BookmarkAppNavigationThrottleResult::kCount);
}

scoped_refptr<const Extension> GetAppForWindow(content::WebContents* source) {
  SCOPED_UMA_HISTOGRAM_TIMER("Extensions.BookmarkApp.GetAppForWindowDuration");
  content::BrowserContext* context = source->GetBrowserContext();
  Browser* browser = chrome::FindBrowserWithWebContents(source);
  if (!browser || !browser->is_app())
    return nullptr;

  const Extension* app = ExtensionRegistry::Get(context)->GetExtensionById(
      web_app::GetExtensionIdFromApplicationName(browser->app_name()),
      extensions::ExtensionRegistry::ENABLED);

  if (!IsWindowedBookmarkApp(app, context))
    return nullptr;

  // Bookmark Apps for installable websites have scope.
  // TODO(crbug.com/774918): Replace once there is a more explicit indicator
  // of a Bookmark App for an installable website.
  if (UrlHandlers::GetUrlHandlers(app) == nullptr)
    return nullptr;

  return app;
}

scoped_refptr<const Extension> GetTargetApp(content::WebContents* source,
                                            const GURL& target_url) {
  SCOPED_UMA_HISTOGRAM_TIMER("Extensions.BookmarkApp.GetTargetAppDuration");
  return extensions::util::GetInstalledPwaForUrl(
      source->GetBrowserContext(), target_url,
      extensions::LAUNCH_CONTAINER_WINDOW);
}

scoped_refptr<const Extension> GetAppForMainFrameURL(
    content::WebContents* source) {
  SCOPED_UMA_HISTOGRAM_TIMER(
      "Extensions.BookmarkApp.GetAppForCurrentURLDuration");
  return extensions::util::GetInstalledPwaForUrl(
      source->GetBrowserContext(),
      source->GetMainFrame()->GetLastCommittedURL(),
      extensions::LAUNCH_CONTAINER_WINDOW);
}

void OpenNewForegroundTab(content::NavigationHandle* navigation_handle) {
  content::WebContents* source = navigation_handle->GetWebContents();
  content::OpenURLParams url_params(navigation_handle->GetURL(),
                                    navigation_handle->GetReferrer(),
                                    WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                    navigation_handle->GetPageTransition(),
                                    navigation_handle->IsRendererInitiated());
  // Per the "Submit as entity body" algorithm
  // (https://html.spec.whatwg.org/#submit-body), POST form submissions include
  // the Content-Type header, so we forward it.
  if (navigation_handle->GetResourceRequestBody()) {
    std::string content_type;
    navigation_handle->GetRequestHeaders().GetHeader(
        net::HttpRequestHeaders::kContentType, &content_type);

    net::HttpRequestHeaders content_type_header;
    content_type_header.SetHeader(net::HttpRequestHeaders::kContentType,
                                  content_type);

    url_params.extra_headers = content_type_header.ToString();
  }

  url_params.uses_post = navigation_handle->IsPost();
  url_params.post_data = navigation_handle->GetResourceRequestBody();
  url_params.redirect_chain = navigation_handle->GetRedirectChain();
  url_params.frame_tree_node_id = navigation_handle->GetFrameTreeNodeId();
  url_params.user_gesture = navigation_handle->HasUserGesture();
  url_params.started_from_context_menu =
      navigation_handle->WasStartedFromContextMenu();

  source->OpenURL(url_params);
}

void ReparentIntoPopup(content::WebContents* source, bool has_user_gesture) {
  Browser* source_browser = chrome::FindBrowserWithWebContents(source);

  Browser::CreateParams browser_params(
      Browser::TYPE_POPUP, source_browser->profile(), has_user_gesture);
  browser_params.initial_bounds = source_browser->override_bounds();
  Browser* popup_browser = new Browser(browser_params);
  TabStripModel* source_tabstrip = source_browser->tab_strip_model();
  popup_browser->tab_strip_model()->AppendWebContents(
      source_tabstrip->DetachWebContentsAt(source_tabstrip->active_index()),
      true /* foreground */);
  popup_browser->window()->Show();
}

}  // namespace extensions

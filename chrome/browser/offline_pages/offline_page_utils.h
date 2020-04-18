// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_UTILS_H_
#define CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_UTILS_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/files/file_util.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "url/gurl.h"

namespace base {
class Time;
}

namespace content {
class BrowserContext;
class NavigationEntry;
class WebContents;
}

namespace offline_pages {
struct OfflinePageHeader;
struct OfflinePageItem;

class OfflinePageUtils {
 public:
  // The result of checking duplicate page or request.
  enum class DuplicateCheckResult {
    // Page with same URL is found.
    DUPLICATE_PAGE_FOUND,
    // Request with same URL is found.
    DUPLICATE_REQUEST_FOUND,
    // No page or request with same URL is found.
    NOT_FOUND
  };

  // Controls the UI action that could be triggered during download.
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.offlinepages
  // GENERATED_JAVA_CLASS_NAME_OVERRIDE: DownloadUiActionFlags
  enum class DownloadUIActionFlags {
    NONE = 0x0,
    // Shows an infobar to prompt the user for re-download when the duplicate
    // page or request is found.
    PROMPT_DUPLICATE = 0x1,
    // Shows a toast when the new download starts.
    SHOW_TOAST_ON_NEW_DOWNLOAD = 0x2,
    // All actions.
    ALL = 0xFFFF
  };

  static const base::FilePath::CharType kMHTMLExtension[];

  // Callback to inform the duplicate checking result.
  using DuplicateCheckCallback = base::Callback<void(DuplicateCheckResult)>;

  // Returns via callback all offline pages related to |url|. The offline page
  // captured from last visit in the tab will be excluded if its tab id does not
  // match the provided |tab_id|. The returned list is sorted based creation
  // date in descending order. That is, the most recently created offline will
  // appear as the first element of the list.
  static void SelectPagesForURL(
      content::BrowserContext* browser_context,
      const GURL& url,
      URLSearchMode url_search_mode,
      int tab_id,
      const base::Callback<void(const std::vector<OfflinePageItem>&)>&
          callback);

  // Gets the offline page corresponding to the given web contents.  The
  // returned pointer is owned by the web_contents and may be deleted by user
  // navigation, so it is unsafe to store a copy of the returned pointer.
  static const OfflinePageItem* GetOfflinePageFromWebContents(
      content::WebContents* web_contents);

  // Gets the offline header provided when loading the offline page for the
  // given web contents.
  static const OfflinePageHeader* GetOfflineHeaderFromWebContents(
      content::WebContents* web_contents);

  // Returns true if the offline page is shown for previewing purpose.
  static bool IsShowingOfflinePreview(content::WebContents* web_contents);

  // Returns true if download button is shown in the error page.
  static bool IsShowingDownloadButtonInErrorPage(
      content::WebContents* web_contents);

  // Gets an Android Tab ID from a tab containing |web_contents|. Returns false,
  // when tab is not available. Returns true otherwise and sets |tab_id| to the
  // ID of the tab.
  static bool GetTabId(content::WebContents* web_contents, int* tab_id);

  // Returns true if the |web_contents| is currently being presented inside a
  // custom tab.
  static bool CurrentlyShownInCustomTab(content::WebContents* web_contents);

  static bool EqualsIgnoringFragment(const GURL& lhs, const GURL& rhs);

  // Returns original URL of the given web contents. Empty URL is returned if
  // no redirect occurred.
  static GURL GetOriginalURLFromWebContents(content::WebContents* web_contents);

  // Checks for duplicates in all finished or ongoing downloads. Only pages and
  // requests that could result in showing in Download UI are searched for
  // |url| match. Result is returned in |callback|. See DuplicateCheckCallback
  // for more details.
  static void CheckDuplicateDownloads(content::BrowserContext* browser_context,
                                      const GURL& url,
                                      const DuplicateCheckCallback& callback);

  // Shows appropriate UI to indicate to the user that the |url| is either
  // already downloaded or is already scheduled to be downloaded soon (as
  // indicated by |exists_duplicate_request|). The user either decides to
  // continue with creating a duplicate - which is indicated by invoking the
  // |confirm_continuation|, or cancels the whole operation which does not
  // invoke continuation then.
  static void ShowDuplicatePrompt(const base::Closure& confirm_continuation,
                                  const GURL& url,
                                  bool exists_duplicate_request,
                                  content::WebContents* web_contents);

  // Shows temporary UI indicating that download was accepted and has started.
  static void ShowDownloadingToast();

  // Schedules to download a page from |url| and categorize under |name_space|.
  // The duplicate pages or requests will be checked. Note that |url| can be
  // different from the one rendered in |web_contents|.
  static void ScheduleDownload(content::WebContents* web_contents,
                               const std::string& name_space,
                               const GURL& url,
                               DownloadUIActionFlags ui_action,
                               const std::string& request_origin);

  static void ScheduleDownload(content::WebContents* web_contents,
                               const std::string& name_space,
                               const GURL& url,
                               DownloadUIActionFlags ui_action);

  // Determines if offline page download should be triggered based on MIME type
  // of download resource.
  static bool CanDownloadAsOfflinePage(const GURL& url,
                                       const std::string& contents_mime_type);

  // Get total size of cache offline pages for a given time range. Returns false
  // when an OfflinePageModel cannot be acquired using the |browser_context|, or
  // the time range is invalid (|begin_time| > |end_time|). Also returning false
  // means no callback should be expected.
  static bool GetCachedOfflinePageSizeBetween(
      content::BrowserContext* browser_context,
      const SizeInBytesCallback& callback,
      const base::Time& begin_time,
      const base::Time& end_time);

  // Extracts and returns the value of the custom offline header from a
  // navigation entry. Empty string is returned if it is not found.
  // Note that the offline header is assumed to be the onlt extra header if it
  // exists.
  static std::string ExtractOfflineHeaderValueFromNavigationEntry(
      const content::NavigationEntry& entry);

  // Returns true if |web_contents| is showing a trusted offline page.
  static bool IsShowingTrustedOfflinePage(content::WebContents* web_contents);

  // Tries to acquires the file access permission. |callback| will be called
  // to inform if the file access permission is granted.
  static void AcquireFileAccessPermission(
      content::WebContents* web_contents,
      const base::Callback<void(bool)>& callback);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_UTILS_H_

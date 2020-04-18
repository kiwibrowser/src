// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/offline_page_utils.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/net/net_error_tab_helper.h"
#include "chrome/browser/offline_pages/offline_page_mhtml_archiver.h"
#include "chrome/browser/offline_pages/offline_page_model_factory.h"
#include "chrome/browser/offline_pages/offline_page_origin_utils.h"
#include "chrome/browser/offline_pages/offline_page_tab_helper.h"
#include "chrome/browser/offline_pages/request_coordinator_factory.h"
#include "components/offline_pages/core/background/request_coordinator.h"
#include "components/offline_pages/core/background/save_page_request.h"
#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/client_policy_controller.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/request_header/offline_page_header.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/mime_util.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/download/download_controller_base.h"
#endif  // defined(OS_ANDROID)

namespace offline_pages {
namespace {

class OfflinePageComparer {
 public:
  OfflinePageComparer() = default;

  bool operator()(const OfflinePageItem& a, const OfflinePageItem& b) {
    return a.creation_time > b.creation_time;
  }
};

void OnGetPagesByURLDone(
    const GURL& url,
    int tab_id,
    const std::vector<std::string>& namespaces_to_show_in_original_tab,
    const base::Callback<void(const std::vector<OfflinePageItem>&)>& callback,
    const MultipleOfflinePageItemResult& pages) {
  std::vector<OfflinePageItem> selected_pages;
  std::string tab_id_str = base::IntToString(tab_id);

  // Exclude pages whose tab id does not match.
  for (const auto& page : pages) {
    if (base::ContainsValue(namespaces_to_show_in_original_tab,
                            page.client_id.name_space) &&
        page.client_id.id != tab_id_str) {
      continue;
    }
    selected_pages.push_back(page);
  }

  // Sort based on creation date.
  std::sort(selected_pages.begin(), selected_pages.end(),
            OfflinePageComparer());

  callback.Run(selected_pages);
}

bool IsSupportedByDownload(content::BrowserContext* browser_context,
                           const std::string& name_space) {
  OfflinePageModel* offline_page_model =
      OfflinePageModelFactory::GetForBrowserContext(browser_context);
  DCHECK(offline_page_model);
  ClientPolicyController* policy_controller =
      offline_page_model->GetPolicyController();
  DCHECK(policy_controller);
  return policy_controller->IsSupportedByDownload(name_space);
}

void CheckDuplicateOngoingDownloads(
    content::BrowserContext* browser_context,
    const GURL& url,
    const OfflinePageUtils::DuplicateCheckCallback& callback) {
  RequestCoordinator* request_coordinator =
      RequestCoordinatorFactory::GetForBrowserContext(browser_context);
  if (!request_coordinator)
    return;

  auto request_coordinator_continuation =
      [](content::BrowserContext* browser_context, const GURL& url,
         const OfflinePageUtils::DuplicateCheckCallback& callback,
         std::vector<std::unique_ptr<SavePageRequest>> requests) {
        base::Time latest_request_time;
        for (auto& request : requests) {
          if (IsSupportedByDownload(browser_context,
                                    request->client_id().name_space) &&
              request->url() == url &&
              latest_request_time < request->creation_time()) {
            latest_request_time = request->creation_time();
          }
        }

        if (latest_request_time.is_null()) {
          callback.Run(OfflinePageUtils::DuplicateCheckResult::NOT_FOUND);
        } else {
          // Using CUSTOM_COUNTS instead of time-oriented histogram to record
          // samples in seconds rather than milliseconds.
          UMA_HISTOGRAM_CUSTOM_COUNTS(
              "OfflinePages.DownloadRequestTimeSinceDuplicateRequested",
              (base::Time::Now() - latest_request_time).InSeconds(),
              base::TimeDelta::FromSeconds(1).InSeconds(),
              base::TimeDelta::FromDays(7).InSeconds(), 50);

          callback.Run(
              OfflinePageUtils::DuplicateCheckResult::DUPLICATE_REQUEST_FOUND);
        }
      };

  request_coordinator->GetAllRequests(base::Bind(
      request_coordinator_continuation, browser_context, url, callback));
}

void DoCalculateSizeBetween(
    const offline_pages::SizeInBytesCallback& callback,
    const base::Time& begin_time,
    const base::Time& end_time,
    const offline_pages::MultipleOfflinePageItemResult& result) {
  int64_t total_size = 0;
  for (auto& page : result) {
    if (begin_time <= page.creation_time && page.creation_time < end_time)
      total_size += page.file_size;
  }
  callback.Run(total_size);
}

content::WebContents* GetWebContentsByFrameID(int render_process_id,
                                              int render_frame_id) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  if (!render_frame_host)
    return NULL;
  return content::WebContents::FromRenderFrameHost(render_frame_host);
}

content::ResourceRequestInfo::WebContentsGetter GetWebContentsGetter(
    content::WebContents* web_contents) {
  // PlzNavigate: The FrameTreeNode ID should be used to access the WebContents.
  int frame_tree_node_id = web_contents->GetMainFrame()->GetFrameTreeNodeId();
  if (frame_tree_node_id != -1) {
    return base::Bind(content::WebContents::FromFrameTreeNodeId,
                      frame_tree_node_id);
  }

  // In other cases, use the RenderProcessHost ID + RenderFrameHost ID to get
  // the WebContents.
  return base::Bind(&GetWebContentsByFrameID,
                    web_contents->GetMainFrame()->GetProcess()->GetID(),
                    web_contents->GetMainFrame()->GetRoutingID());
}

void AcquireFileAccessPermissionDoneForScheduleDownload(
    content::WebContents* web_contents,
    const std::string& name_space,
    const GURL& url,
    OfflinePageUtils::DownloadUIActionFlags ui_action,
    const std::string& request_origin,
    bool granted) {
  if (!granted)
    return;
  OfflinePageTabHelper* tab_helper =
      OfflinePageTabHelper::FromWebContents(web_contents);
  if (!tab_helper)
    return;
  tab_helper->ScheduleDownloadHelper(web_contents, name_space, url, ui_action,
                                     request_origin);
}

}  // namespace

// static
const base::FilePath::CharType OfflinePageUtils::kMHTMLExtension[] =
    FILE_PATH_LITERAL("mhtml");

// static
void OfflinePageUtils::SelectPagesForURL(
    content::BrowserContext* browser_context,
    const GURL& url,
    URLSearchMode url_search_mode,
    int tab_id,
    const base::Callback<void(const std::vector<OfflinePageItem>&)>& callback) {
  OfflinePageModel* offline_page_model =
      OfflinePageModelFactory::GetForBrowserContext(browser_context);
  if (!offline_page_model) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(callback, std::vector<OfflinePageItem>()));
    return;
  }

  offline_page_model->GetPagesByURL(
      url,
      url_search_mode,
      base::Bind(&OnGetPagesByURLDone, url, tab_id,
                 offline_page_model->GetPolicyController()
                     ->GetNamespacesRestrictedToOriginalTab(),
                 callback));
}

const OfflinePageItem* OfflinePageUtils::GetOfflinePageFromWebContents(
    content::WebContents* web_contents) {
  OfflinePageTabHelper* tab_helper =
      OfflinePageTabHelper::FromWebContents(web_contents);
  if (!tab_helper)
    return nullptr;
  const OfflinePageItem* offline_page = tab_helper->offline_page();
  if (!offline_page)
    return nullptr;

  // If a pending navigation that hasn't committed yet, don't return the cached
  // offline page that was set at the last commit time. This is to prevent
  // from returning the wrong offline page if DidStartNavigation is never called
  // to clear it up.
  if (!EqualsIgnoringFragment(web_contents->GetVisibleURL(),
                              web_contents->GetLastCommittedURL())) {
    return nullptr;
  }

  return offline_page;
}

// static
const OfflinePageHeader* OfflinePageUtils::GetOfflineHeaderFromWebContents(
    content::WebContents* web_contents) {
  OfflinePageTabHelper* tab_helper =
      OfflinePageTabHelper::FromWebContents(web_contents);
  return tab_helper ? &(tab_helper->offline_header()) : nullptr;
}

// static
bool OfflinePageUtils::IsShowingOfflinePreview(
    content::WebContents* web_contents) {
  OfflinePageTabHelper* tab_helper =
      OfflinePageTabHelper::FromWebContents(web_contents);
  return tab_helper && tab_helper->GetOfflinePreviewItem();
}

// static
bool OfflinePageUtils::IsShowingDownloadButtonInErrorPage(
    content::WebContents* web_contents) {
  chrome_browser_net::NetErrorTabHelper* tab_helper =
      chrome_browser_net::NetErrorTabHelper::FromWebContents(web_contents);
  return tab_helper && tab_helper->is_showing_download_button_in_error_page();
}

// static
bool OfflinePageUtils::EqualsIgnoringFragment(const GURL& lhs,
                                              const GURL& rhs) {
  GURL::Replacements remove_params;
  remove_params.ClearRef();

  GURL lhs_stripped = lhs.ReplaceComponents(remove_params);
  GURL rhs_stripped = rhs.ReplaceComponents(remove_params);

  return lhs_stripped == rhs_stripped;
}

// static
GURL OfflinePageUtils::GetOriginalURLFromWebContents(
    content::WebContents* web_contents) {
  content::NavigationEntry* entry =
      web_contents->GetController().GetLastCommittedEntry();
  if (!entry || entry->GetRedirectChain().size() <= 1)
    return GURL();
  return entry->GetRedirectChain().front();
}

// static
void OfflinePageUtils::CheckDuplicateDownloads(
    content::BrowserContext* browser_context,
    const GURL& url,
    const DuplicateCheckCallback& callback) {
  // First check for finished downloads, that is, saved pages.
  OfflinePageModel* offline_page_model =
      OfflinePageModelFactory::GetForBrowserContext(browser_context);
  if (!offline_page_model)
    return;

  auto continuation = [](content::BrowserContext* browser_context,
                         const GURL& url,
                         const DuplicateCheckCallback& callback,
                         const std::vector<OfflinePageItem>& pages) {
    base::Time latest_saved_time;
    for (const auto& offline_page_item : pages) {
      if (IsSupportedByDownload(browser_context,
                                offline_page_item.client_id.name_space) &&
          latest_saved_time < offline_page_item.creation_time) {
        latest_saved_time = offline_page_item.creation_time;
      }
    }
    if (latest_saved_time.is_null()) {
      // Then check for ongoing downloads, that is, requests.
      CheckDuplicateOngoingDownloads(browser_context, url, callback);
    } else {
      // Using CUSTOM_COUNTS instead of time-oriented histogram to record
      // samples in seconds rather than milliseconds.
      UMA_HISTOGRAM_CUSTOM_COUNTS(
          "OfflinePages.DownloadRequestTimeSinceDuplicateSaved",
          (base::Time::Now() - latest_saved_time).InSeconds(),
          base::TimeDelta::FromSeconds(1).InSeconds(),
          base::TimeDelta::FromDays(7).InSeconds(), 50);

      callback.Run(DuplicateCheckResult::DUPLICATE_PAGE_FOUND);
    }
  };

  offline_page_model->GetPagesByURL(
      url, URLSearchMode::SEARCH_BY_ALL_URLS,
      base::Bind(continuation, browser_context, url, callback));
}

// static
void OfflinePageUtils::ScheduleDownload(content::WebContents* web_contents,
                                        const std::string& name_space,
                                        const GURL& url,
                                        DownloadUIActionFlags ui_action,
                                        const std::string& request_origin) {
  DCHECK(web_contents);

  // Ensure that the storage permission is granted since the archive file is
  // going to be placed in the public directory.
  AcquireFileAccessPermission(
      web_contents,
      base::Bind(&AcquireFileAccessPermissionDoneForScheduleDownload,
                 web_contents, name_space, url, ui_action, request_origin));
}

// static
void OfflinePageUtils::ScheduleDownload(content::WebContents* web_contents,
                                        const std::string& name_space,
                                        const GURL& url,
                                        DownloadUIActionFlags ui_action) {
  std::string origin =
      OfflinePageOriginUtils::GetEncodedOriginAppFor(web_contents);
  ScheduleDownload(web_contents, name_space, url, ui_action, origin);
}

// static
bool OfflinePageUtils::CanDownloadAsOfflinePage(
    const GURL& url,
    const std::string& contents_mime_type) {
  return url.SchemeIsHTTPOrHTTPS() &&
         (net::MatchesMimeType(contents_mime_type, "text/html") ||
          net::MatchesMimeType(contents_mime_type, "application/xhtml+xml"));
}

// static
bool OfflinePageUtils::GetCachedOfflinePageSizeBetween(
    content::BrowserContext* browser_context,
    const SizeInBytesCallback& callback,
    const base::Time& begin_time,
    const base::Time& end_time) {
  OfflinePageModel* offline_page_model =
      OfflinePageModelFactory::GetForBrowserContext(browser_context);
  if (!offline_page_model || begin_time > end_time)
    return false;
  offline_page_model->GetPagesRemovedOnCacheReset(
      base::Bind(&DoCalculateSizeBetween, callback, begin_time, end_time));
  return true;
}

// static
std::string OfflinePageUtils::ExtractOfflineHeaderValueFromNavigationEntry(
    const content::NavigationEntry& entry) {
  std::string extra_headers = entry.GetExtraHeaders();
  if (extra_headers.empty())
    return std::string();

  // The offline header will be the only extra header if it is present.
  std::string offline_header_key(offline_pages::kOfflinePageHeader);
  offline_header_key += ": ";
  if (!base::StartsWith(extra_headers, offline_header_key,
                        base::CompareCase::INSENSITIVE_ASCII)) {
    return std::string();
  }
  std::string header_value = extra_headers.substr(offline_header_key.length());
  if (header_value.find("\n") != std::string::npos)
    return std::string();

  return header_value;
}

// static
bool OfflinePageUtils::IsShowingTrustedOfflinePage(
    content::WebContents* web_contents) {
  OfflinePageTabHelper* tab_helper =
      OfflinePageTabHelper::FromWebContents(web_contents);
  return tab_helper && tab_helper->IsShowingTrustedOfflinePage();
}

// static
void OfflinePageUtils::AcquireFileAccessPermission(
    content::WebContents* web_contents,
    const base::Callback<void(bool)>& callback) {
#if defined(OS_ANDROID)
  content::ResourceRequestInfo::WebContentsGetter web_contents_getter =
      GetWebContentsGetter(web_contents);
  DownloadControllerBase::Get()->AcquireFileAccessPermission(
      web_contents_getter, callback);
#else
  // Not needed in other platforms.
  callback.Run(true /*granted*/);
#endif  // defined(OS_ANDROID)
}

}  // namespace offline_pages

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/previews/previews_infobar_tab_helper.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "chrome/browser/loader/chrome_navigation_data.h"
#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings.h"
#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings_factory.h"
#include "chrome/browser/previews/previews_infobar_delegate.h"
#include "chrome/browser/previews/previews_service.h"
#include "chrome/browser/previews/previews_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_service.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_settings.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h"
#include "components/offline_pages/buildflags/buildflags.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/previews/content/previews_content_util.h"
#include "components/previews/content/previews_ui_service.h"
#include "components/previews/core/previews_experiments.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
#include "chrome/browser/offline_pages/offline_page_tab_helper.h"
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)

namespace {

// Adds the preview navigation to the black list.
void AddPreviewNavigationCallback(content::BrowserContext* browser_context,
                                  const GURL& url,
                                  previews::PreviewsType type,
                                  uint64_t page_id,
                                  bool opt_out) {
  PreviewsService* previews_service = PreviewsServiceFactory::GetForProfile(
      Profile::FromBrowserContext(browser_context));
  if (previews_service && previews_service->previews_ui_service()) {
    previews_service->previews_ui_service()->AddPreviewNavigation(
        url, type, opt_out, page_id);
  }
}

}  // namespace

DEFINE_WEB_CONTENTS_USER_DATA_KEY(PreviewsInfoBarTabHelper);

PreviewsInfoBarTabHelper::~PreviewsInfoBarTabHelper() {}

PreviewsInfoBarTabHelper::PreviewsInfoBarTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      displayed_preview_infobar_(false),
      displayed_preview_timestamp_(false) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
}

void PreviewsInfoBarTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Only show the infobar if this is a full main frame navigation.
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() || navigation_handle->IsSameDocument())
    return;

  previews_user_data_.reset();
  // Store Previews information for this navigation.
  ChromeNavigationData* nav_data = static_cast<ChromeNavigationData*>(
      navigation_handle->GetNavigationData());
  if (nav_data && nav_data->previews_user_data()) {
    previews_user_data_ = nav_data->previews_user_data()->DeepCopy();
  }

  uint64_t page_id = (previews_user_data_) ? previews_user_data_->page_id() : 0;

  // The infobar should only be told if the page was a reload if the previous
  // page displayed a timestamp.
  bool is_reload =
      displayed_preview_timestamp_
          ? navigation_handle->GetReloadType() != content::ReloadType::NONE
          : false;
  displayed_preview_infobar_ = false;
  displayed_preview_timestamp_ = false;

  // Retrieve PreviewsUIService* from |web_contents| if available.
  PreviewsService* previews_service = PreviewsServiceFactory::GetForProfile(
      Profile::FromBrowserContext(web_contents()->GetBrowserContext()));
  previews::PreviewsUIService* previews_ui_service =
      previews_service ? previews_service->previews_ui_service() : nullptr;

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  offline_pages::OfflinePageTabHelper* tab_helper =
      offline_pages::OfflinePageTabHelper::FromWebContents(web_contents());

  if (tab_helper && tab_helper->GetOfflinePreviewItem()) {
    if (navigation_handle->IsErrorPage()) {
      // TODO(ryansturm): Add UMA for errors.
      return;
    }
    data_reduction_proxy::DataReductionProxySettings*
        data_reduction_proxy_settings =
            DataReductionProxyChromeSettingsFactory::GetForBrowserContext(
                web_contents()->GetBrowserContext());

    const offline_pages::OfflinePageItem* offline_page =
        tab_helper->GetOfflinePreviewItem();
    // From UMA, the median percent of network body bytes loaded out of total
    // body bytes on a page load. See PageLoad.Experimental.Bytes.Network and
    // PageLoad.Experimental.Bytes.Total.
    int64_t uncached_size = offline_page->file_size * 0.55;

    bool data_saver_enabled =
        data_reduction_proxy_settings->IsDataReductionProxyEnabled();

    data_reduction_proxy_settings->data_reduction_proxy_service()
        ->UpdateDataUseForHost(0, uncached_size,
                               navigation_handle->GetRedirectChain()[0].host());

    data_reduction_proxy_settings->data_reduction_proxy_service()
        ->UpdateContentLengths(0, uncached_size, data_saver_enabled,
                               data_reduction_proxy::HTTPS,
                               "multipart/related");

    PreviewsInfoBarDelegate::Create(
        web_contents(), previews::PreviewsType::OFFLINE,
        base::Time() /* previews_freshness */,
        data_reduction_proxy_settings && data_saver_enabled,
        false /* is_reload */,
        base::BindOnce(&AddPreviewNavigationCallback,
                       web_contents()->GetBrowserContext(),
                       navigation_handle->GetRedirectChain()[0],
                       previews::PreviewsType::OFFLINE, page_id),
        previews_ui_service);
    // Don't try to show other infobars if this is an offline preview.
    return;
  }
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)

  // Check for committed main frame preview.
  if (previews_user_data_ && previews_user_data_->HasCommittedPreviewsType()) {
    previews::PreviewsType main_frame_preview =
        previews_user_data_->committed_previews_type();
    if (main_frame_preview != previews::PreviewsType::NONE &&
        main_frame_preview != previews::PreviewsType::LOFI) {
      base::Time previews_freshness;
      if (main_frame_preview == previews::PreviewsType::LITE_PAGE) {
        const net::HttpResponseHeaders* headers =
            navigation_handle->GetResponseHeaders();
        if (headers)
          headers->GetDateValue(&previews_freshness);
      }

      PreviewsInfoBarDelegate::Create(
          web_contents(), main_frame_preview, previews_freshness,
          true /* is_data_saver_user */, is_reload,
          base::BindOnce(&AddPreviewNavigationCallback,
                         web_contents()->GetBrowserContext(),
                         navigation_handle->GetRedirectChain()[0],
                         main_frame_preview, page_id),
          previews_ui_service);
    }
  }
}

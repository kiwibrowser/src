// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_update_request_base.h"
#include "content/browser/appcache/appcache_update_url_loader_request.h"
#include "content/browser/appcache/appcache_update_url_request.h"
#include "content/public/common/content_features.h"
#include "net/url_request/url_request_context.h"
#include "services/network/public/cpp/features.h"

namespace content {

namespace {
constexpr net::NetworkTrafficAnnotationTag kAppCacheTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("appcache_update_job", R"(
      semantics {
        sender: "HTML5 AppCache System"
        description:
          "Web pages can include a link to a manifest file which lists "
          "resources to be cached for offline access. The AppCache system"
          "retrieves those resources in the background."
        trigger:
          "User visits a web page containing a <html manifest=manifestUrl> "
          "tag, or navigates to a document retrieved from an existing appcache "
          "and some resource should be updated."
        data: "None"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: YES
        cookies_store: "user"
        setting:
          "Users can control this feature via the 'Cookies' setting under "
          "'Privacy, Content settings'. If cookies are disabled for a single "
          "site, appcaches are disabled for the site only. If they are totally "
          "disabled, all appcache requests will be stopped."
        chrome_policy {
            DefaultCookiesSetting {
              DefaultCookiesSetting: 2
            }
          }
      })");
}

AppCacheUpdateJob::UpdateRequestBase::~UpdateRequestBase() {}

// static
std::unique_ptr<AppCacheUpdateJob::UpdateRequestBase>
AppCacheUpdateJob::UpdateRequestBase::Create(
    AppCacheServiceImpl* appcache_service,
    const GURL& url,
    int buffer_size,
    URLFetcher* fetcher) {
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    return std::unique_ptr<UpdateRequestBase>(new UpdateURLRequest(
        appcache_service->request_context(), url, buffer_size, fetcher));
  } else {
    return std::unique_ptr<UpdateRequestBase>(new UpdateURLLoaderRequest(
        appcache_service->url_loader_factory_getter(), url, buffer_size,
        fetcher));
  }
}

AppCacheUpdateJob::UpdateRequestBase::UpdateRequestBase() {}

net::NetworkTrafficAnnotationTag
AppCacheUpdateJob::UpdateRequestBase::GetTrafficAnnotation() {
  return kAppCacheTrafficAnnotation;
}

}  // namespace content

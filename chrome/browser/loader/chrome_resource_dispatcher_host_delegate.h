// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LOADER_CHROME_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
#define CHROME_BROWSER_LOADER_CHROME_RESOURCE_DISPATCHER_HOST_DELEGATE_H_

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"
#include "content/public/common/previews_state.h"
#include "extensions/buildflags/buildflags.h"

class DownloadRequestLimiter;

namespace content {
class NavigationData;
}

namespace extensions {
class UserScriptListener;
}

namespace net {
class URLRequest;
}

namespace previews {
class PreviewsDecider;
}

namespace safe_browsing {
class SafeBrowsingService;
}

// Implements ResourceDispatcherHostDelegate. Currently used by the Prerender
// system to abort requests and add to the load flags when a request begins.
class ChromeResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:
  ChromeResourceDispatcherHostDelegate();
  ~ChromeResourceDispatcherHostDelegate() override;

  // ResourceDispatcherHostDelegate implementation.
  void RequestBeginning(net::URLRequest* request,
                        content::ResourceContext* resource_context,
                        content::AppCacheService* appcache_service,
                        content::ResourceType resource_type,
                        std::vector<std::unique_ptr<content::ResourceThrottle>>*
                            throttles) override;
  void DownloadStarting(net::URLRequest* request,
                        content::ResourceContext* resource_context,
                        bool is_content_initiated,
                        bool must_download,
                        bool is_new_request,
                        std::vector<std::unique_ptr<content::ResourceThrottle>>*
                            throttles) override;
  bool ShouldInterceptResourceAsStream(net::URLRequest* request,
                                       const std::string& mime_type,
                                       GURL* origin,
                                       std::string* payload) override;
  void OnStreamCreated(net::URLRequest* request,
                       std::unique_ptr<content::StreamInfo> stream) override;
  void OnResponseStarted(net::URLRequest* request,
                         content::ResourceContext* resource_context,
                         network::ResourceResponse* response) override;
  void OnRequestRedirected(const GURL& redirect_url,
                           net::URLRequest* request,
                           content::ResourceContext* resource_context,
                           network::ResourceResponse* response) override;
  void RequestComplete(net::URLRequest* url_request) override;
  // Returns a bitmask of potentially several Previews optimizations at the
  // start of a navigation.
  content::PreviewsState DetermineEnabledPreviews(
      net::URLRequest* url_request,
      content::ResourceContext* resource_context,
      content::PreviewsState previews_to_allow) override;
  content::NavigationData* GetNavigationData(
      net::URLRequest* request) const override;

 protected:
  // Virtual for testing.
  virtual void AppendStandardResourceThrottles(
      net::URLRequest* request,
      content::ResourceContext* resource_context,
      content::ResourceType resource_type,
      std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles);

 private:
  friend class ChromeResourceDispatcherHostDelegateTest;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  struct StreamTargetInfo {
    std::string extension_id;
    std::string view_id;
  };
#endif

  // Returns an updated bitmask of Previews for a committed navigation.
  // The value is updated from |intial_state| considering navigation data
  // attached to |request|.
  static content::PreviewsState DetermineCommittedPreviews(
      const net::URLRequest* request,
      const previews::PreviewsDecider* previews_decider,
      content::PreviewsState initial_state);

  scoped_refptr<DownloadRequestLimiter> download_request_limiter_;
  scoped_refptr<safe_browsing::SafeBrowsingService> safe_browsing_;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  scoped_refptr<extensions::UserScriptListener> user_script_listener_;
  std::map<net::URLRequest*, StreamTargetInfo> stream_target_info_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ChromeResourceDispatcherHostDelegate);
};

#endif  // CHROME_BROWSER_LOADER_CHROME_RESOURCE_DISPATCHER_HOST_DELEGATE_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_URL_REQUEST_INTERCEPTOR_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_URL_REQUEST_INTERCEPTOR_H_

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/unguessable_token.h"
#include "content/browser/devtools/devtools_network_interceptor.h"
#include "content/browser/devtools/devtools_target_registry.h"
#include "content/public/common/resource_type.h"
#include "net/url_request/url_request_interceptor.h"

namespace content {

class BrowserContext;
class DevToolsInterceptorController;
class DevToolsTargetRegistry;
class DevToolsURLInterceptorRequestJob;
class ResourceRequestInfo;

// An interceptor that creates DevToolsURLInterceptorRequestJobs for requests
// from pages where interception has been enabled via
// Network.setRequestInterceptionEnabled.
// This class is constructed on the UI thread but only accessed on IO thread.
class DevToolsURLRequestInterceptor : public net::URLRequestInterceptor,
                                      public DevToolsNetworkInterceptor {
 public:
  static bool IsNavigationRequest(ResourceType resource_type);

  explicit DevToolsURLRequestInterceptor(BrowserContext* browser_context);
  ~DevToolsURLRequestInterceptor() override;

  // DevToolsNetworkInterceptor implementation.
  void AddFilterEntry(std::unique_ptr<FilterEntry> entry) override;
  void RemoveFilterEntry(const FilterEntry* entry) override;
  void UpdatePatterns(FilterEntry* entry,
                      std::vector<Pattern> patterns) override;
  void GetResponseBody(std::string interception_id,
                       std::unique_ptr<GetResponseBodyForInterceptionCallback>
                           callback) override;
  void ContinueInterceptedRequest(
      std::string interception_id,
      std::unique_ptr<Modifications> modifications,
      std::unique_ptr<ContinueInterceptedRequestCallback> callback) override;

  // net::URLRequestInterceptor implementation.
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  net::URLRequestJob* MaybeInterceptRedirect(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      const GURL& location) const override;

  net::URLRequestJob* MaybeInterceptResponse(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  // Registers a |sub_request| that should not be intercepted.
  void RegisterSubRequest(const net::URLRequest* sub_request);
  void UnregisterSubRequest(const net::URLRequest* sub_request);
  // To make the user's life easier we make sure requests in a redirect chain
  // all have the same id.
  void ExpectRequestAfterRedirect(const net::URLRequest* request,
                                  std::string id);
  void JobFinished(const std::string& interception_id, bool is_navigation);

 private:
  net::URLRequestJob* InnerMaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate);

  FilterEntry* FilterEntryForRequest(base::UnguessableToken target_id,
                                     const GURL& url,
                                     ResourceType resource_type,
                                     InterceptionStage* stage);

  const DevToolsTargetRegistry::TargetInfo* TargetInfoForRequestInfo(
      const ResourceRequestInfo* request_info) const;

  std::string GetIdForRequest(const net::URLRequest* request,
                              bool* is_redirect);
  // Returns a WeakPtr to the DevToolsURLInterceptorRequestJob corresponding
  // to |interception_id|.
  DevToolsURLInterceptorRequestJob* GetJob(
      const std::string& interception_id) const;

  std::unique_ptr<DevToolsTargetRegistry::Resolver> target_resolver_;
  base::WeakPtr<DevToolsInterceptorController> controller_;

  base::flat_map<base::UnguessableToken,
                 std::vector<std::unique_ptr<FilterEntry>>>
      target_id_to_entries_;

  base::flat_map<std::string, DevToolsURLInterceptorRequestJob*>
      interception_id_to_job_map_;

  // The DevToolsURLInterceptorRequestJob proxies a sub request to actually
  // fetch the bytes from the network. We don't want to intercept those
  // requests.
  base::flat_set<const net::URLRequest*> sub_requests_;

  // To simplify handling of redirect chains for the end user, we arrange for
  // all requests in the chain to have the same interception id.
  base::flat_map<const net::URLRequest*, std::string> expected_redirects_;
  size_t next_id_;

  base::WeakPtrFactory<DevToolsURLRequestInterceptor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsURLRequestInterceptor);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_URL_REQUEST_INTERCEPTOR_H_

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_NETWORK_REQUEST_FACTORY_IMPL_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_NETWORK_REQUEST_FACTORY_IMPL_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "components/offline_pages/core/prefetch/prefetch_dispatcher.h"
#include "components/offline_pages/core/prefetch/prefetch_network_request_factory.h"
#include "components/version_info/channel.h"
#include "net/url_request/url_request_context_getter.h"

namespace offline_pages {
class GeneratePageBundleRequest;
class GetOperationRequest;

class PrefetchNetworkRequestFactoryImpl : public PrefetchNetworkRequestFactory {
 public:
  PrefetchNetworkRequestFactoryImpl(
      net::URLRequestContextGetter* request_context,
      version_info::Channel channel,
      const std::string& user_agent);

  ~PrefetchNetworkRequestFactoryImpl() override;

  bool HasOutstandingRequests() const override;

  void MakeGeneratePageBundleRequest(
      const std::vector<std::string>& prefetch_urls,
      const std::string& gcm_registration_id,
      const PrefetchRequestFinishedCallback& callback) override;

  std::unique_ptr<std::set<std::string>> GetAllUrlsRequested() const override;

  void MakeGetOperationRequest(
      const std::string& operation_name,
      const PrefetchRequestFinishedCallback& callback) override;

  GetOperationRequest* FindGetOperationRequestByName(
      const std::string& operation_name) const override;

  std::unique_ptr<std::set<std::string>> GetAllOperationNamesRequested()
      const override;

 private:
  void GeneratePageBundleRequestDone(
      const PrefetchRequestFinishedCallback& callback,
      uint64_t request_id,
      PrefetchRequestStatus status,
      const std::string& operation_name,
      const std::vector<RenderPageInfo>& pages);

  void GetOperationRequestDone(const PrefetchRequestFinishedCallback& callback,
                               PrefetchRequestStatus status,
                               const std::string& operation_name,
                               const std::vector<RenderPageInfo>& pages);

  // Returns 'true' if a new network request can be created, 'false' if the
  // maximum count of concurrent network requests has been reached.
  bool AddConcurrentRequest();

  // Called when network request finishes, decrementing the counter of
  // concurrent network requests.
  void ReleaseConcurrentRequest();

  uint64_t GetNextRequestId();

  scoped_refptr<net::URLRequestContextGetter> request_context_;
  version_info::Channel channel_;
  std::string user_agent_;

  std::map<uint64_t, std::unique_ptr<GeneratePageBundleRequest>>
      generate_page_bundle_requests_;
  std::map<std::string, std::unique_ptr<GetOperationRequest>>
      get_operation_requests_;

  // Count of concurrent network requests of any type. Used as emergency limiter
  // to prevent generating too many parallel network requests.
  size_t concurrent_request_count_ = 0;
  // Used to id GeneratePageBundle requests so they can be removed from the map.
  uint64_t request_id_ = 0;

  base::WeakPtrFactory<PrefetchNetworkRequestFactoryImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchNetworkRequestFactoryImpl);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_NETWORK_REQUEST_FACTORY_IMPL_H_

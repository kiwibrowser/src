// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_TEST_TEST_URL_LOADER_FACTORY_H_
#define SERVICES_NETWORK_TEST_TEST_URL_LOADER_FACTORY_H_

#include <map>
#include <vector>

#include "base/macros.h"
#include "net/http/http_status_code.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace network {

// A helper class to ease testing code that uses URLLoader interface. A test
// would pass this factory instead of the production factory to code, and
// would prime it with response data for arbitrary URLs.
class TestURLLoaderFactory : public mojom::URLLoaderFactory {
 public:
  TestURLLoaderFactory();
  ~TestURLLoaderFactory() override;

  using Redirects =
      std::vector<std::pair<net::RedirectInfo, ResourceResponseHead>>;

  // Adds a response to be served. There is one unique response per URL, and if
  // this method is called multiple times for the same URL the last response
  // data is used.
  // This can be called before or after a request is made. If it's called after,
  // then pending requests will be "woken up".
  void AddResponse(const GURL& url,
                   const ResourceResponseHead& head,
                   const std::string& content,
                   const URLLoaderCompletionStatus& status,
                   const Redirects& redirects = Redirects());

  // Simpler version of above for the common case of success or error page.
  void AddResponse(const std::string& url,
                   const std::string& content,
                   net::HttpStatusCode status = net::HTTP_OK);

  // Returns true if there is a request for a given URL with a living client
  // that did not produce a response yet. If |load_flags_out| is non-null,
  // it will reports load flags used for the request
  // WARNING: This does RunUntilIdle() first.
  bool IsPending(const std::string& url, int* load_flags_out = nullptr);

  // Returns the total # of pending requests.
  // WARNING: This does RunUntilIdle() first.
  int NumPending();

  // Clear all the responses that were previously set.
  void ClearResponses();

  using Interceptor = base::RepeatingCallback<void(const ResourceRequest&)>;
  void SetInterceptor(const Interceptor& interceptor);

  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& url_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(mojom::URLLoaderFactoryRequest request) override;

 private:
  bool CreateLoaderAndStartInternal(const GURL& url,
                                    mojom::URLLoaderClient* client);

  struct Response {
    Response();
    ~Response();
    Response(const Response&);
    GURL url;
    Redirects redirects;
    ResourceResponseHead head;
    std::string content;
    URLLoaderCompletionStatus status;
  };
  std::map<GURL, Response> responses_;

  struct Pending {
    Pending();
    ~Pending();
    Pending(Pending&& other);
    Pending& operator=(Pending&& other);
    GURL url;
    int load_flags;
    mojom::URLLoaderClientPtr client;
  };
  std::vector<Pending> pending_;

  Interceptor interceptor_;

  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderFactory);
};

}  // namespace network

#endif  // SERVICES_NETWORK_TEST_TEST_URL_LOADER_FACTORY_H_

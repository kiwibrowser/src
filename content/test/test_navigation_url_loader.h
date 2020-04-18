// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_TEST_NAVIGATION_URL_LOADER_H_
#define CONTENT_TEST_TEST_NAVIGATION_URL_LOADER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "content/browser/frame_host/navigation_request_info.h"
#include "content/browser/loader/navigation_url_loader.h"
#include "content/common/navigation_params.h"

namespace net {
struct RedirectInfo;
}

namespace network {
struct ResourceResponse;
}

namespace content {

class NavigationData;
class NavigationURLLoaderDelegate;

// Test implementation of NavigationURLLoader to simulate the network stack
// response.
class TestNavigationURLLoader
    : public NavigationURLLoader,
      public base::SupportsWeakPtr<TestNavigationURLLoader> {
 public:
  TestNavigationURLLoader(std::unique_ptr<NavigationRequestInfo> request_info,
                          NavigationURLLoaderDelegate* delegate);

  // NavigationURLLoader implementation.
  void FollowRedirect() override;
  void ProceedWithResponse() override;

  NavigationRequestInfo* request_info() const { return request_info_.get(); }

  void SimulateServerRedirect(const GURL& redirect_url);

  void SimulateError(int error_code);

  void CallOnRequestRedirected(
      const net::RedirectInfo& redirect_info,
      const scoped_refptr<network::ResourceResponse>& response);
  void CallOnResponseStarted(
      const scoped_refptr<network::ResourceResponse>& response,
      std::unique_ptr<NavigationData> navigation_data);

  int redirect_count() { return redirect_count_; }

  bool response_proceeded() { return response_proceeded_; }

 private:
  ~TestNavigationURLLoader() override;

  std::unique_ptr<NavigationRequestInfo> request_info_;
  NavigationURLLoaderDelegate* delegate_;
  int redirect_count_;
  bool response_proceeded_;
};

}  // namespace content

#endif  // CONTENT_TEST_TEST_NAVIGATION_URL_LOADER_H_

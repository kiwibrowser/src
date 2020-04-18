// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_DEVTOOLS_PROXY_H_
#define CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_DEVTOOLS_PROXY_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/unguessable_token.h"
#include "content/common/content_export.h"
#include "services/network/public/cpp/resource_response.h"

class GURL;

namespace base {
class UnguessableToken;
}  // namespace base

namespace net {
class SSLInfo;
}  // namespace net

namespace network {
struct ResourceRequest;
struct ResourceResponseHead;
struct URLLoaderCompletionStatus;
}  // namespace network

namespace content {
class SignedExchangeHeader;

// SignedExchangeDevToolsProxy lives on the IO thread and sends messages to
// DevTools via the UI thread to show signed exchange related information.
class CONTENT_EXPORT SignedExchangeDevToolsProxy {
 public:
  // |frame_tree_node_id_getter| callback will be called on the UI thread to get
  // the frame tree node ID. Note: We are using callback beause when Network
  // Service is not enabled the ID is not available while handling prefetch
  // requests on the IO thread.
  // When the signed exchange request is a navigation request,
  // |devtools_navigation_token| can be used to find the matching request in
  // DevTools. But when the signed exchange request is a prefetch request, the
  // browser process doesn't know the request id used in DevTools. So DevTools
  // looks up the inflight requests using |outer_request_url| to find the
  // matching request.
  SignedExchangeDevToolsProxy(
      const GURL& outer_request_url,
      const network::ResourceResponseHead& outer_response,
      base::RepeatingCallback<int(void)> frame_tree_node_id_getter,
      base::Optional<const base::UnguessableToken> devtools_navigation_token,
      bool report_raw_headers);
  ~SignedExchangeDevToolsProxy();

  void ReportErrorMessage(const std::string& message);
  void CertificateRequestSent(const base::UnguessableToken& request_id,
                              const network::ResourceRequest& request);
  void CertificateResponseReceived(const base::UnguessableToken& request_id,
                                   const GURL& url,
                                   const network::ResourceResponseHead& head);
  void CertificateRequestCompleted(
      const base::UnguessableToken& request_id,
      const network::URLLoaderCompletionStatus& status);

  void OnSignedExchangeReceived(
      const base::Optional<SignedExchangeHeader>& header,
      const net::SSLInfo* ssl_info);

 private:
  const GURL outer_request_url_;
  const network::ResourceResponseHead outer_response_;
  const base::RepeatingCallback<int(void)> frame_tree_node_id_getter_;
  const base::Optional<const base::UnguessableToken> devtools_navigation_token_;
  const bool devtools_enabled_;
  std::vector<std::string> error_messages_;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeDevToolsProxy);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_DEVTOOLS_PROXY_H_

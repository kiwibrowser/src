// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_DELEGATE_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_DELEGATE_H_

#include "base/macros.h"

#include "base/synchronization/lock.h"
#include "headless/public/headless_browser_context.h"
#include "net/base/network_delegate_impl.h"

namespace headless {
class HeadlessBrowserContextImpl;

// We use the HeadlessNetworkDelegate to remove DevTools request headers before
// requests are actually fetched and for reporting failed network requests.
class HeadlessNetworkDelegate : public net::NetworkDelegateImpl,
                                public HeadlessBrowserContext::Observer {
 public:
  explicit HeadlessNetworkDelegate(
      HeadlessBrowserContextImpl* headless_browser_context);
  ~HeadlessNetworkDelegate() override;

 private:
  // net::NetworkDelegateImpl implementation:
  int OnBeforeURLRequest(net::URLRequest* request,
                         const net::CompletionCallback& callback,
                         GURL* new_url) override;

  int OnBeforeStartTransaction(net::URLRequest* request,
                               const net::CompletionCallback& callback,
                               net::HttpRequestHeaders* headers) override;

  void OnStartTransaction(net::URLRequest* request,
                          const net::HttpRequestHeaders& headers) override;

  int OnHeadersReceived(
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;

  void OnBeforeRedirect(net::URLRequest* request,
                        const GURL& new_location) override;

  void OnResponseStarted(net::URLRequest* request, int net_error) override;

  void OnCompleted(net::URLRequest* request,
                   bool started,
                   int net_error) override;

  void OnURLRequestDestroyed(net::URLRequest* request) override;

  void OnPACScriptError(int line_number, const base::string16& error) override;

  NetworkDelegate::AuthRequiredResponse OnAuthRequired(
      net::URLRequest* request,
      const net::AuthChallengeInfo& auth_info,
      const AuthCallback& callback,
      net::AuthCredentials* credentials) override;

  bool OnCanGetCookies(const net::URLRequest& request,
                       const net::CookieList& cookie_list) override;

  bool OnCanSetCookie(const net::URLRequest& request,
                      const net::CanonicalCookie& cookie,
                      net::CookieOptions* options) override;

  bool OnCanAccessFile(const net::URLRequest& request,
                       const base::FilePath& original_path,
                       const base::FilePath& absolute_path) const override;

  // HeadlessBrowserContext::Observer implementation:
  void OnHeadlessBrowserContextDestruct() override;

  base::Lock lock_;  // Protects |headless_browser_context_|.
  HeadlessBrowserContextImpl* headless_browser_context_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(HeadlessNetworkDelegate);
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_NETWORK_DELEGATE_H_

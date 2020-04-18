// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_SHELL_NETWORK_DELEGATE_H_
#define EXTENSIONS_SHELL_BROWSER_SHELL_NETWORK_DELEGATE_H_

#include "base/macros.h"
#include "extensions/browser/info_map.h"
#include "net/base/network_delegate_impl.h"

namespace extensions {

class InfoMap;

class ShellNetworkDelegate : public net::NetworkDelegateImpl {
 public:
  ShellNetworkDelegate(void* browser_context, InfoMap* extension_info_map);
  ~ShellNetworkDelegate() override;

  static void SetAcceptAllCookies(bool accept);

 private:
  // NetworkDelegate implementation.
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
  net::NetworkDelegate::AuthRequiredResponse OnAuthRequired(
      net::URLRequest* request,
      const net::AuthChallengeInfo& auth_info,
      const AuthCallback& callback,
      net::AuthCredentials* credentials) override;

  void* browser_context_;
  scoped_refptr<extensions::InfoMap> extension_info_map_;

  DISALLOW_COPY_AND_ASSIGN(ShellNetworkDelegate);
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_SHELL_NETWORK_DELEGATE_H_

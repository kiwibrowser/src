// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_
#define SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_

#include "base/macros.h"
#include "net/base/network_delegate_impl.h"

namespace network {

class NetworkContext;

class NetworkServiceNetworkDelegate : public net::NetworkDelegateImpl {
 public:
  // |network_context| is guaranteed to outlive this class.
  explicit NetworkServiceNetworkDelegate(NetworkContext* network_context);

 private:
  // net::NetworkDelegateImpl implementation.
  bool OnCanGetCookies(const net::URLRequest& request,
                       const net::CookieList& cookie_list) override;
  bool OnCanSetCookie(const net::URLRequest& request,
                      const net::CanonicalCookie& cookie,
                      net::CookieOptions* options) override;
  bool OnCanAccessFile(const net::URLRequest& request,
                       const base::FilePath& original_path,
                       const base::FilePath& absolute_path) const override;

  NetworkContext* network_context_;

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceNetworkDelegate);
};

}  // namespace network

#endif  // SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_

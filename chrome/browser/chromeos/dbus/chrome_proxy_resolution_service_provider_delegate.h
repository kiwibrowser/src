// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DBUS_CHROME_PROXY_RESOLUTION_SERVICE_PROVIDER_DELEGATE_H_
#define CHROME_BROWSER_CHROMEOS_DBUS_CHROME_PROXY_RESOLUTION_SERVICE_PROVIDER_DELEGATE_H_

#include "base/macros.h"
#include "chromeos/dbus/services/proxy_resolution_service_provider.h"

namespace chromeos {

// Chrome's implementation of ProxyResolutionServiceProvider::Delegate.
class ChromeProxyResolutionServiceProviderDelegate
    : public ProxyResolutionServiceProvider::Delegate {
 public:
  ChromeProxyResolutionServiceProviderDelegate();
  ~ChromeProxyResolutionServiceProviderDelegate() override;

  // ProxyResolutionServiceProvider::Delegate:
  scoped_refptr<net::URLRequestContextGetter> GetRequestContext() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeProxyResolutionServiceProviderDelegate);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DBUS_CHROME_PROXY_RESOLUTION_SERVICE_PROVIDER_DELEGATE_H_

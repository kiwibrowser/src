// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_DHCP_PAC_FILE_FETCHER_FACTORY_CHROMEOS_H_
#define CHROMEOS_NETWORK_DHCP_PAC_FILE_FETCHER_FACTORY_CHROMEOS_H_

#include <memory>

#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "net/proxy_resolution/dhcp_pac_file_fetcher_factory.h"

namespace net {
class DhcpPacFileFetcher;
class URLRequestContext;
}

namespace chromeos {

// ChromeOS specific implementation of DhcpPacFileFetcherFactory.
// TODO(mmenke):  This won't work at all with an out-of-process network service.
// Figure out a way forward there.
class CHROMEOS_EXPORT DhcpPacFileFetcherFactoryChromeos
    : public net::DhcpPacFileFetcherFactory {
 public:
  DhcpPacFileFetcherFactoryChromeos();
  ~DhcpPacFileFetcherFactoryChromeos() override;

  // net::DhcpPacFileFetcherFactory implementation.
  std::unique_ptr<net::DhcpPacFileFetcher> Create(
      net::URLRequestContext* url_request_context) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DhcpPacFileFetcherFactoryChromeos);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_DHCP_PAC_FILE_FETCHER_FACTORY_CHROMEOS_H_

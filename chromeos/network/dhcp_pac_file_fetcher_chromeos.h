// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_DHCP_PAC_FILE_FETCHER_CHROMEOS_H_
#define CHROMEOS_NETWORK_DHCP_PAC_FILE_FETCHER_CHROMEOS_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/chromeos_export.h"
#include "net/proxy_resolution/dhcp_pac_file_fetcher.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace net {
class URLRequestContext;
class NetLogWithSource;
class PacFileFetcher;
}

namespace chromeos {

// ChromeOS specific implementation of DhcpPacFileFetcher.
// This looks up Service.WebProxyAutoDiscoveryUrl for the default network
// from Shill and uses that to fetch the PAC file if available.
class CHROMEOS_EXPORT DhcpPacFileFetcherChromeos
    : public net::DhcpPacFileFetcher {
 public:
  explicit DhcpPacFileFetcherChromeos(
      net::URLRequestContext* url_request_context);
  ~DhcpPacFileFetcherChromeos() override;

  // net::DhcpPacFileFetcher
  int Fetch(base::string16* utf16_text,
            const net::CompletionCallback& callback,
            const net::NetLogWithSource& net_log,
            const net::NetworkTrafficAnnotationTag traffic_annotation) override;
  void Cancel() override;
  void OnShutdown() override;
  const GURL& GetPacURL() const override;
  std::string GetFetcherName() const override;

 private:
  void ContinueFetch(base::string16* utf16_text,
                     net::CompletionCallback callback,
                     const net::NetworkTrafficAnnotationTag traffic_annotation,
                     std::string pac_url);

  std::unique_ptr<net::PacFileFetcher> pac_file_fetcher_;
  scoped_refptr<base::SingleThreadTaskRunner> network_handler_task_runner_;

  GURL pac_url_;

  base::WeakPtrFactory<DhcpPacFileFetcherChromeos> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DhcpPacFileFetcherChromeos);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_DHCP_PAC_FILE_FETCHER_CHROMEOS_H_
